#include "fontbake.hpp"

#include "font.hpp"
#include "ktx2.hpp"

#include <fmt/base.h>
#include <stb_rect_pack.h>

// clang-format off
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#include FT_MODULE_H
#include FT_OUTLINE_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
// clang-format on

#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {
namespace font = sponge::scene::font;
namespace ktx2 = sponge::scene::ktx2;

constexpr uint32_t atlasSize     = 1024;
constexpr uint32_t atlasChannels = 3;

// Printable ASCII plus U+00D7 MULTIPLICATION SIGN.
std::vector<char32_t> charset() {
    std::vector<char32_t> codepoints;
    for (char32_t codepoint = 32; codepoint <= 126; codepoint++) {
        codepoints.push_back(codepoint);
    }
    codepoints.push_back(0xD7);
    return codepoints;
}

// Characters that appear in tabular-figure strings (percent, resolution,
// aspect ratio). Only these have their tnum glyphs rasterized.
constexpr std::array<char32_t, 15> tabularCandidates = {
    { U'0', U'1', U'2', U'3', U'4', U'5', U'6', U'7', U'8', U'9', U':', U'%',
      U'~', U' ', 0xD7 }
};

// liga, clig and calt off: shaping must never ask for a glyph the atlas does
// not hold (Inter's calt swaps multiply for multiply.case). tnum only on
// request.
std::array<hb_feature_t, 4> featuresFor(const bool tabularFigures) {
    const auto feature = [](const hb_tag_t tag, const uint32_t value) {
        return hb_feature_t{
            .tag = tag, .value = value, .start = 0, .end = UINT_MAX
        };
    };
    return { feature(HB_TAG('l', 'i', 'g', 'a'), 0),
             feature(HB_TAG('c', 'l', 'i', 'g'), 0),
             feature(HB_TAG('c', 'a', 'l', 't'), 0),
             feature(HB_TAG('t', 'n', 'u', 'm'), tabularFigures ? 1 : 0) };
}

// charset() tops out at U+00D7, so two-byte sequences suffice.
std::string toUtf8(const std::span<const char32_t> codepoints) {
    std::string text;
    for (const auto codepoint : codepoints) {
        assert(codepoint < 0x800);
        if (codepoint < 0x80) {
            text += static_cast<char>(codepoint);
        } else {
            text += static_cast<char>(0xC0 | (codepoint >> 6));
            text += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    }
    return text;
}

struct HarfBuzzGlyph {
    uint32_t glyphIndex;
    int32_t  xAdvance;
    int32_t  xOffset;
    int32_t  yOffset;
};

// The same call BitmapFont made at run time before shaping was baked.
std::vector<HarfBuzzGlyph> shapeWithHarfBuzz(hb_font_t*         hbFont,
                                             const std::string& text,
                                             const bool tabularFigures) {
    const std::unique_ptr<hb_buffer_t, decltype(&hb_buffer_destroy)> buffer{
        hb_buffer_create(), &hb_buffer_destroy
    };
    hb_buffer_add_utf8(buffer.get(), text.data(), static_cast<int>(text.size()),
                       0, static_cast<int>(text.size()));
    hb_buffer_guess_segment_properties(buffer.get());
    const auto features = featuresFor(tabularFigures);
    hb_shape(hbFont, buffer.get(), features.data(),
             static_cast<unsigned int>(features.size()));

    unsigned int count    = 0;
    const auto*  infos    = hb_buffer_get_glyph_infos(buffer.get(), &count);
    const auto* positions = hb_buffer_get_glyph_positions(buffer.get(), &count);

    std::vector<HarfBuzzGlyph> glyphs;
    for (unsigned int i = 0; i < count; i++) {
        glyphs.push_back({ .glyphIndex = infos[i].codepoint,
                           .xAdvance   = positions[i].x_advance,
                           .xOffset    = positions[i].x_offset,
                           .yOffset    = positions[i].y_offset });
    }
    return glyphs;
}

uint64_t glyphKey(const uint32_t glyphIndex, const uint32_t size,
                  const uint32_t phase) {
    return (static_cast<uint64_t>(glyphIndex) << 32) | (size << 2) | phase;
}

struct PendingGlyph {
    uint32_t             glyphIndex = 0;
    uint32_t             size       = 0;
    uint32_t             phase      = 0;
    font::Glyph          glyph{};
    std::vector<uint8_t> bitmap;
};

struct Rasterized {
    std::vector<PendingGlyph> pending;
    std::vector<stbrp_rect>   rects;
    // Glyphs with no bitmap, such as space, need no atlas room.
    std::map<uint64_t, font::Glyph> placed;
};

void rasterize(FT_Face face, const uint32_t glyphIndex, const uint32_t size,
               const uint32_t phase, Rasterized& out) {
    if (glyphIndex == 0 ||
        FT_Load_Glyph(face, glyphIndex,
                      FT_LOAD_TARGET_LIGHT | FT_LOAD_NO_BITMAP) != 0) {
        return;
    }

    const FT_GlyphSlot slot = face->glyph;
    // Shift the outline by phase/4 pixel (26.6 fixed point) so each phase
    // bakes a distinct subpixel position.
    FT_Outline_Translate(&slot->outline, static_cast<FT_Pos>(phase * 16), 0);
    if (FT_Render_Glyph(slot, FT_RENDER_MODE_LCD) != 0) {
        return;
    }
    const int width  = static_cast<int>(slot->bitmap.width) / 3;
    const int height = static_cast<int>(slot->bitmap.rows);
    const int pitch  = std::abs(slot->bitmap.pitch);

    font::Glyph glyph{};
    glyph.bearingX = slot->bitmap_left;
    glyph.bearingY = slot->bitmap_top;
    glyph.width    = width;
    glyph.height   = height;

    if (width == 0 || height == 0) {
        out.placed[glyphKey(glyphIndex, size, phase)] = glyph;
        return;
    }

    PendingGlyph pending{ .glyphIndex = glyphIndex,
                          .size       = size,
                          .phase      = phase,
                          .glyph      = glyph,
                          .bitmap     = {} };
    pending.bitmap.resize(static_cast<size_t>(width) * atlasChannels * height);
    for (int row = 0; row < height; row++) {
        std::copy_n(slot->bitmap.buffer + (row * pitch), width * atlasChannels,
                    pending.bitmap.begin() + (row * width * atlasChannels));
    }

    stbrp_rect rect{};
    rect.id = static_cast<int>(out.rects.size());
    rect.w  = width + 1;
    rect.h  = height + 1;
    out.rects.push_back(rect);
    out.pending.push_back(std::move(pending));
}

// Packs every pending bitmap into the atlas and records where it went.
std::optional<std::vector<uint8_t>> pack(Rasterized&        rasterized,
                                         const std::string& path) {
    std::vector<uint8_t> atlas(static_cast<size_t>(atlasSize) * atlasSize *
                               atlasChannels);

    std::vector<stbrp_node> nodes(atlasSize);
    stbrp_context           context{};
    stbrp_init_target(&context, atlasSize, atlasSize, nodes.data(),
                      static_cast<int>(nodes.size()));
    stbrp_pack_rects(&context, rasterized.rects.data(),
                     static_cast<int>(rasterized.rects.size()));

    for (size_t i = 0; i < rasterized.pending.size(); i++) {
        const auto& pending = rasterized.pending[i];
        const auto& rect    = rasterized.rects[i];
        if (rect.was_packed == 0) {
            fmt::println(stderr,
                         "assetconv: {}: glyph {} at {} px does not fit a "
                         "{}x{} atlas",
                         path, pending.glyphIndex, pending.size, atlasSize,
                         atlasSize);
            return std::nullopt;
        }

        const auto width = static_cast<size_t>(pending.glyph.width);
        for (int row = 0; row < pending.glyph.height; row++) {
            std::copy_n(pending.bitmap.begin() + (row * width * atlasChannels),
                        width * atlasChannels,
                        atlas.begin() + ((((static_cast<size_t>(rect.y) + row) *
                                           atlasSize) +
                                          rect.x) *
                                         atlasChannels));
        }

        auto glyph     = pending.glyph;
        glyph.uvLeft   = static_cast<float>(rect.x) / atlasSize;
        glyph.uvTop    = static_cast<float>(rect.y) / atlasSize;
        glyph.uvWidth  = static_cast<float>(pending.glyph.width) / atlasSize;
        glyph.uvHeight = static_cast<float>(pending.glyph.height) / atlasSize;
        rasterized
            .placed[glyphKey(pending.glyphIndex, pending.size, pending.phase)] =
            glyph;
    }
    return atlas;
}

// Glyph index to slot, in order of first use. Slot 0 is the missing glyph.
struct Slots {
    std::map<uint32_t, uint32_t> ofGlyph{ { 0, 0 } };
    std::vector<uint32_t>        glyphs{ 0 };

    uint32_t at(const uint32_t glyphIndex) {
        const auto [entry, inserted] = ofGlyph.try_emplace(
            glyphIndex, static_cast<uint32_t>(glyphs.size()));
        if (inserted) {
            glyphs.push_back(glyphIndex);
        }
        return entry->second;
    }
};

// Single-codepoint shaping gives the glyph for each codepoint and the plain
// advance of each glyph. Pair shaping gives the kerning.
bool buildTables(hb_font_t* hbFont, FT_Face face,
                 const std::vector<uint32_t>& sizes, const Rasterized& placed,
                 const std::string& path, Slots& slots, font::Font& out) {
    const auto codepoints = charset();

    for (const auto pixels : sizes) {
        FT_Set_Pixel_Sizes(face, 0, pixels);
        hb_ft_font_changed(hbFont);

        font::Size size{
            .pixels     = pixels,
            .lineHeight = static_cast<float>(face->size->metrics.height >> 6),
            .ascender   = static_cast<float>(face->size->metrics.ascender >> 6),
            .advances   = {},
            .glyphs     = {},
            .kerning    = {},
        };
        std::map<uint32_t, int32_t> advances{ { 0, hb_font_get_glyph_h_advance(
                                                       hbFont, 0) } };

        for (const bool tabular : { false, true }) {
            auto& map = tabular ? out.tabularSlots : out.slots;
            for (const auto codepoint : codepoints) {
                const auto shaped = shapeWithHarfBuzz(
                    hbFont, toUtf8({ &codepoint, 1 }), tabular);
                if (shaped.size() != 1 || shaped[0].xOffset != 0 ||
                    shaped[0].yOffset != 0) {
                    fmt::println(stderr,
                                 "assetconv: {}: U+{:04X} does not shape to "
                                 "one unshifted glyph",
                                 path, static_cast<uint32_t>(codepoint));
                    return false;
                }
                const auto slot = slots.at(shaped[0].glyphIndex);
                map[codepoint]  = slot;

                const auto [entry, inserted] =
                    advances.try_emplace(slot, shaped[0].xAdvance);
                if (!inserted && entry->second != shaped[0].xAdvance) {
                    fmt::println(stderr,
                                 "assetconv: {}: glyph {} advances differently "
                                 "with tabular figures",
                                 path, shaped[0].glyphIndex);
                    return false;
                }
            }
        }

        for (const bool tabular : { false, true }) {
            const auto& map = tabular ? out.tabularSlots : out.slots;
            for (const auto first : codepoints) {
                for (const auto second : codepoints) {
                    const std::array pair{ first, second };
                    const auto       shaped =
                        shapeWithHarfBuzz(hbFont, toUtf8(pair), tabular);
                    const auto firstSlot  = map.at(first);
                    const auto secondSlot = map.at(second);
                    if (shaped.size() != 2 || shaped[0].xOffset != 0 ||
                        shaped[0].yOffset != 0 || shaped[1].xOffset != 0 ||
                        shaped[1].yOffset != 0 ||
                        shaped[1].xAdvance != advances.at(secondSlot)) {
                        fmt::println(stderr,
                                     "assetconv: {}: U+{:04X} U+{:04X} needs "
                                     "more than pair kerning",
                                     path, static_cast<uint32_t>(first),
                                     static_cast<uint32_t>(second));
                        return false;
                    }
                    const auto adjustment =
                        shaped[0].xAdvance - advances.at(firstSlot);
                    const auto key = (firstSlot << 16) | secondSlot;
                    const auto [entry, inserted] =
                        size.kerning.try_emplace(key, adjustment);
                    if (!inserted && entry->second != adjustment) {
                        fmt::println(stderr,
                                     "assetconv: {}: glyph pair kerns "
                                     "differently with tabular figures",
                                     path);
                        return false;
                    }
                }
            }
        }
        std::erase_if(size.kerning,
                      [](const auto& entry) { return entry.second == 0; });

        size.advances.resize(slots.glyphs.size());
        size.glyphs.resize(slots.glyphs.size() * font::subpixelPhases);
        for (uint32_t slot = 0; slot < slots.glyphs.size(); slot++) {
            size.advances[slot] = advances.at(slot);
            for (uint32_t phase = 0; phase < font::subpixelPhases; phase++) {
                const auto found = placed.placed.find(
                    glyphKey(slots.glyphs[slot], pixels, phase));
                if (found != placed.placed.end()) {
                    size.glyphs[(slot * font::subpixelPhases) + phase] =
                        found->second;
                }
            }
        }
        out.sizes.push_back(std::move(size));
    }

    out.slotCount = static_cast<uint32_t>(slots.glyphs.size());
    return true;
}

// Shapes random strings both ways at every size. A font whose shaping
// depends on more than the previous glyph fails here.
bool matchesHarfBuzz(hb_font_t* hbFont, FT_Face face, const font::Font& baked,
                     const Slots& slots, const std::string& path) {
    constexpr uint32_t stringCount = 256;
    constexpr uint32_t maxLength   = 48;

    const auto   codepoints  = charset();
    const auto&  slotOfGlyph = slots.ofGlyph;
    std::mt19937 random{ 1 };  // fixed: the bake must be reproducible

    for (const auto& size : baked.sizes) {
        FT_Set_Pixel_Sizes(face, 0, size.pixels);
        hb_ft_font_changed(hbFont);

        for (uint32_t i = 0; i < stringCount; i++) {
            std::vector<char32_t> text(1 + (random() % maxLength));
            for (auto& codepoint : text) {
                codepoint = codepoints[random() % codepoints.size()];
            }
            const auto utf8 = toUtf8(text);

            for (const bool tabular : { false, true }) {
                const auto want = shapeWithHarfBuzz(hbFont, utf8, tabular);
                const auto got  = font::shape(baked, size, utf8, tabular);
                bool       same = want.size() == got.size();
                for (size_t g = 0; same && g < want.size(); g++) {
                    const auto slot = slotOfGlyph.find(want[g].glyphIndex);
                    same = slot != slotOfGlyph.end() &&
                           slot->second == got[g].slot &&
                           static_cast<float>(want[g].xAdvance) / 64.F ==
                               got[g].xAdvance &&
                           want[g].xOffset == 0 && want[g].yOffset == 0;
                }
                if (!same) {
                    fmt::println(stderr,
                                 "assetconv: {}: baked shaping differs from "
                                 "HarfBuzz at {} px for \"{}\"",
                                 path, size.pixels, utf8);
                    return false;
                }
            }
        }
    }
    return true;
}
}  // namespace

namespace assetconv {

std::vector<uint8_t> bakeFont(const std::string&           path,
                              const std::vector<uint32_t>& sizes) {
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0) {
        fmt::println(stderr, "assetconv: FreeType init failed");
        return {};
    }
    const std::unique_ptr<FT_LibraryRec_, decltype(&FT_Done_FreeType)>
        libraryOwner{ library, &FT_Done_FreeType };

    FT_Library_SetLcdFilter(library, FT_LCD_FILTER_DEFAULT);

    // Thicken stems to make up for coverage lost to gamma-unaware blending.
    constexpr FT_Bool noStemDarkening = 0;
    FT_Property_Set(library, "autofitter", "no-stem-darkening",
                    &noStemDarkening);

    FT_Face face = nullptr;
    if (FT_New_Face(library, path.c_str(), 0, &face) != 0) {
        fmt::println(stderr, "assetconv: unable to load font {}", path);
        return {};
    }
    const std::unique_ptr<FT_FaceRec_, decltype(&FT_Done_Face)> faceOwner{
        face, &FT_Done_Face
    };

    // Referenced: the hb font holds its own reference to the face.
    const std::unique_ptr<hb_font_t, decltype(&hb_font_destroy)> hbFont{
        hb_ft_font_create_referenced(face), &hb_font_destroy
    };
    // Unhinted metrics keep advances fractional; light hinting would round
    // them to whole pixels and defeat subpixel positioning.
    hb_ft_font_set_load_flags(hbFont.get(), FT_LOAD_NO_HINTING);

    Rasterized rasterized;
    for (const auto size : sizes) {
        FT_Set_Pixel_Sizes(face, 0, size);
        hb_ft_font_changed(hbFont.get());

        // tnum glyphs (Inter also swaps some punctuation, such as ':') are
        // outside the cmap, so shaping finds them.
        const auto tabular =
            shapeWithHarfBuzz(hbFont.get(), toUtf8(tabularCandidates), true);

        for (uint32_t phase = 0; phase < font::subpixelPhases; phase++) {
            for (const auto codepoint : charset()) {
                rasterize(face, FT_Get_Char_Index(face, codepoint), size, phase,
                          rasterized);
            }
            for (const auto& glyph : tabular) {
                rasterize(face, glyph.glyphIndex, size, phase, rasterized);
            }
        }
    }

    auto atlas = pack(rasterized, path);
    if (!atlas) {
        return {};
    }

    Slots      slots;
    font::Font baked;
    if (!buildTables(hbFont.get(), face, sizes, rasterized, path, slots,
                     baked) ||
        !matchesHarfBuzz(hbFont.get(), face, baked, slots, path)) {
        return {};
    }

    const std::vector<ktx2::KeyValue> keyValues{ { std::string{ font::fontKey },
                                                   font::write(baked) } };
    return ktx2::write(ktx2::formatR8G8B8Unorm, atlasSize, atlasSize,
                       std::vector<std::vector<uint8_t>>{ std::move(*atlas) },
                       keyValues);
}

}  // namespace assetconv
