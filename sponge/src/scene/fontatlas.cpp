#include "scene/fontatlas.hpp"

#include "logging/log.hpp"

#include <stb_rect_pack.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <vector>

namespace sponge::scene {

namespace {
constexpr uint32_t atlasSize  = 1024;
constexpr char32_t firstGlyph = 32;
constexpr char32_t lastGlyph  = 126;

constexpr std::array<char32_t, 1> supplementalGlyphs = {
    0xD7,  // × MULTIPLICATION SIGN
};
}  // namespace

FontAtlas::~FontAtlas() {
    if (hbFont != nullptr) {
        hb_font_destroy(hbFont);
    }
    if (ftFace != nullptr) {
        FT_Done_Face(ftFace);
    }
    if (ftLibrary != nullptr) {
        FT_Done_FreeType(ftLibrary);
    }
}

void FontAtlas::build(const std::string&           path,
                      const std::vector<uint32_t>& sizes) {
    assert(textureWidth == 0 && "FontAtlas::build() called more than once");

    if (FT_Init_FreeType(&ftLibrary) != 0) {
        SPONGE_CORE_ERROR("FreeType init failed");
        return;
    }

    FT_Library_SetLcdFilter(ftLibrary, FT_LCD_FILTER_DEFAULT);

    // thicken stems to compensate for coverage loss in gamma-unaware blending
    FT_Bool noStemDarkening = 0;
    FT_Property_Set(ftLibrary, "autofitter", "no-stem-darkening",
                    &noStemDarkening);

    if (FT_New_Face(ftLibrary, path.c_str(), 0, &ftFace) != 0) {
        SPONGE_CORE_ERROR("Failed to load font: {}", path);
        return;
    }

    hbFont = hb_ft_font_create_referenced(ftFace);
    // unhinted metrics keep advances fractional; light hinting would
    // round them to whole pixels and defeat subpixel positioning
    hb_ft_font_set_load_flags(hbFont, FT_LOAD_NO_HINTING);

    struct PendingGlyph {
        uint32_t             glyphIndex;
        uint32_t             size;
        uint32_t             phase;
        GlyphInfo            glyphInfo;
        std::vector<uint8_t> bitmap;
        int                  bitmapWidth;
        int                  bitmapHeight;
        int                  rectIdx;
    };

    std::vector<PendingGlyph> pending;
    std::vector<stbrp_rect>   rects;

    for (const uint32_t size : sizes) {
        FT_Set_Pixel_Sizes(ftFace, 0, size);

        lineHeights[size] =
            static_cast<float>(ftFace->size->metrics.height >> 6);
        ascenders[size] =
            static_cast<float>(ftFace->size->metrics.ascender >> 6);

        auto rasterizeGlyph = [&](const char32_t codepoint,
                                  const uint32_t phase) {
            const FT_UInt glyphIndex = FT_Get_Char_Index(ftFace, codepoint);
            if (glyphIndex == 0 ||
                FT_Load_Glyph(ftFace, glyphIndex,
                              FT_LOAD_TARGET_LIGHT | FT_LOAD_NO_BITMAP) != 0) {
                return;
            }

            const FT_GlyphSlot glyphSlot = ftFace->glyph;
            // shift outline by phase/4 pixel (26.6 fixed point) before
            // rasterizing so each phase bakes a distinct subpixel position
            FT_Outline_Translate(&glyphSlot->outline,
                                 static_cast<FT_Pos>(phase * 16), 0);
            if (FT_Render_Glyph(glyphSlot, FT_RENDER_MODE_LCD) != 0) {
                return;
            }
            const int lcdWidth     = static_cast<int>(glyphSlot->bitmap.width);
            const int bitmapPitch  = std::abs(glyphSlot->bitmap.pitch);
            const int bitmapWidth  = lcdWidth / 3;
            const int bitmapHeight = static_cast<int>(glyphSlot->bitmap.rows);

            GlyphInfo glyphInfo{};
            glyphInfo.bearingX = glyphSlot->bitmap_left;
            glyphInfo.bearingY = glyphSlot->bitmap_top;
            glyphInfo.width    = bitmapWidth;
            glyphInfo.height   = bitmapHeight;

            if (bitmapWidth == 0 || bitmapHeight == 0) {
                glyphs[glyphKey(glyphIndex, size, phase)] = glyphInfo;
                return;
            }

            PendingGlyph pendingGlyph;
            pendingGlyph.glyphIndex   = glyphIndex;
            pendingGlyph.size         = size;
            pendingGlyph.phase        = phase;
            pendingGlyph.glyphInfo    = glyphInfo;
            pendingGlyph.bitmapWidth  = bitmapWidth;
            pendingGlyph.bitmapHeight = bitmapHeight;
            pendingGlyph.bitmap.resize(
                static_cast<size_t>(bitmapWidth * 3 * bitmapHeight));
            for (int row = 0; row < bitmapHeight; row++) {
                std::copy(glyphSlot->bitmap.buffer + row * bitmapPitch,
                          glyphSlot->bitmap.buffer + row * bitmapPitch +
                              bitmapWidth * 3,
                          pendingGlyph.bitmap.begin() + row * bitmapWidth * 3);
            }
            pendingGlyph.rectIdx = static_cast<int>(rects.size());
            pending.push_back(std::move(pendingGlyph));

            stbrp_rect rect{};
            rect.id = pending.back().rectIdx;
            rect.w  = static_cast<stbrp_coord>(bitmapWidth + 1);
            rect.h  = static_cast<stbrp_coord>(bitmapHeight + 1);
            rects.push_back(rect);
        };

        for (uint32_t phase = 0; phase < subpixelPhases; phase++) {
            for (char32_t codepoint = firstGlyph; codepoint <= lastGlyph;
                 codepoint++) {
                rasterizeGlyph(codepoint, phase);
            }
            for (const char32_t codepoint : supplementalGlyphs) {
                rasterizeGlyph(codepoint, phase);
            }
        }
    }

    textureWidth  = atlasSize;
    textureHeight = atlasSize;
    atlasBuffer.assign(textureWidth * textureHeight * 3, 0);

    std::vector<stbrp_node> nodes(textureWidth);
    stbrp_context           ctx{};
    stbrp_init_target(&ctx, static_cast<int>(textureWidth),
                      static_cast<int>(textureHeight), nodes.data(),
                      static_cast<int>(nodes.size()));
    stbrp_pack_rects(&ctx, rects.data(), static_cast<int>(rects.size()));

    for (const auto& pendingGlyph : pending) {
        const auto& rect      = rects[pendingGlyph.rectIdx];
        GlyphInfo   glyphInfo = pendingGlyph.glyphInfo;

        if (!rect.was_packed) {
            SPONGE_CORE_WARN("Glyph {} size {} did not fit in atlas",
                             pendingGlyph.glyphIndex, pendingGlyph.size);
            continue;
        }

        for (int row = 0; row < pendingGlyph.bitmapHeight; row++) {
            std::copy(pendingGlyph.bitmap.begin() +
                          row * pendingGlyph.bitmapWidth * 3,
                      pendingGlyph.bitmap.begin() +
                          (row + 1) * pendingGlyph.bitmapWidth * 3,
                      atlasBuffer.begin() +
                          (static_cast<ptrdiff_t>(rect.y + row) *
                               static_cast<ptrdiff_t>(textureWidth) +
                           rect.x) *
                              3);
        }

        glyphInfo.uvLeft =
            static_cast<float>(rect.x) / static_cast<float>(textureWidth);
        glyphInfo.uvTop =
            static_cast<float>(rect.y) / static_cast<float>(textureHeight);
        glyphInfo.uvWidth  = static_cast<float>(pendingGlyph.bitmapWidth) /
                             static_cast<float>(textureWidth);
        glyphInfo.uvHeight = static_cast<float>(pendingGlyph.bitmapHeight) /
                             static_cast<float>(textureHeight);

        glyphs[glyphKey(pendingGlyph.glyphIndex, pendingGlyph.size,
                        pendingGlyph.phase)] = glyphInfo;
    }
}

std::vector<ShapedGlyph> FontAtlas::shape(const std::string_view text,
                                          const uint32_t         size) {
    std::vector<ShapedGlyph> result;

    if (hbFont == nullptr) {
        return result;
    }

    FT_Set_Pixel_Sizes(ftFace, 0, size);
    hb_ft_font_changed(hbFont);

    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, text.data(), static_cast<int>(text.size()), 0,
                       static_cast<int>(text.size()));
    hb_buffer_guess_segment_properties(buffer);

    const hb_feature_t features[] = {
        { HB_TAG('l', 'i', 'g', 'a'), 0, 0, UINT_MAX },
        { HB_TAG('c', 'l', 'i', 'g'), 0, 0, UINT_MAX },
    };
    hb_shape(hbFont, buffer, features, 2);

    unsigned int     glyphCount = 0;
    hb_glyph_info_t* glyphInfos =
        hb_buffer_get_glyph_infos(buffer, &glyphCount);
    hb_glyph_position_t* positions =
        hb_buffer_get_glyph_positions(buffer, &glyphCount);

    result.reserve(glyphCount);
    for (unsigned int index = 0; index < glyphCount; index++) {
        ShapedGlyph shapedGlyph{};
        // after shaping, hb_glyph_info_t.codepoint holds the glyph index
        shapedGlyph.glyphIndex = glyphInfos[index].codepoint;
        shapedGlyph.xAdvance =
            static_cast<float>(positions[index].x_advance) / 64.0F;
        shapedGlyph.xOffset =
            static_cast<float>(positions[index].x_offset) / 64.0F;
        shapedGlyph.yOffset =
            static_cast<float>(positions[index].y_offset) / 64.0F;
        result.push_back(shapedGlyph);
    }

    hb_buffer_destroy(buffer);
    return result;
}

const GlyphInfo* FontAtlas::getGlyph(const uint32_t glyphIndex,
                                     const uint32_t size,
                                     const uint32_t phase) const {
    const auto iter = glyphs.find(glyphKey(glyphIndex, size, phase));
    return iter != glyphs.end() ? &iter->second : nullptr;
}

float FontAtlas::getLineHeight(const uint32_t size) const {
    const auto iter = lineHeights.find(size);
    return iter != lineHeights.end() ? iter->second : static_cast<float>(size);
}

float FontAtlas::getAscender(const uint32_t size) const {
    const auto iter = ascenders.find(size);
    return iter != ascenders.end() ? iter->second : static_cast<float>(size);
}

}  // namespace sponge::scene
