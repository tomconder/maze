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
constexpr uint32_t kAtlasSize  = 512;
constexpr char32_t kFirstGlyph = 32;
constexpr char32_t kLastGlyph  = 126;

constexpr std::array<char32_t, 1> kSupplementalGlyphs = {
    0xD7,  // × MULTIPLICATION SIGN
};
}  // namespace

FontAtlas::~FontAtlas() {
    for (auto& [size, hbFont] : hbFonts) {
        hb_font_destroy(hbFont);
    }
    if (ftFace != nullptr) {
        FT_Done_Face(ftFace);
    }
    if (ftLibrary != nullptr) {
        FT_Done_FreeType(ftLibrary);
    }
}

void FontAtlas::build(const std::vector<FontFaceSpec>& faces,
                      const std::vector<uint32_t>&     sizes) {
    assert(textureWidth == 0 && "FontAtlas::build() called more than once");

    if (FT_Init_FreeType(&ftLibrary) != 0) {
        SPONGE_CORE_ERROR("FreeType init failed");
        return;
    }

    FT_Library_SetLcdFilter(ftLibrary, FT_LCD_FILTER_DEFAULT);

    if (faces.empty()) {
        return;
    }

    if (FT_New_Face(ftLibrary, faces[0].path.c_str(), 0, &ftFace) != 0) {
        SPONGE_CORE_ERROR("Failed to load font: {}", faces[0].path);
        return;
    }

    struct PendingGlyph {
        char32_t             codepoint;
        uint32_t             size;
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

        auto rasterizeGlyph = [&](const char32_t codepoint) {
            if (FT_Load_Char(ftFace, codepoint,
                             FT_LOAD_RENDER | FT_LOAD_TARGET_LCD) != 0) {
                return;
            }

            const FT_GlyphSlot glyphSlot = ftFace->glyph;
            const int lcdWidth     = static_cast<int>(glyphSlot->bitmap.width);
            const int bitmapPitch  = std::abs(glyphSlot->bitmap.pitch);
            const int bitmapWidth  = lcdWidth / 3;
            const int bitmapHeight = static_cast<int>(glyphSlot->bitmap.rows);

            GlyphInfo glyphInfo{};
            glyphInfo.bearingX = glyphSlot->bitmap_left;
            glyphInfo.bearingY = glyphSlot->bitmap_top;
            glyphInfo.width    = bitmapWidth;
            glyphInfo.height   = bitmapHeight;
            glyphInfo.advanceX = static_cast<float>(glyphSlot->advance.x >> 6);

            if (bitmapWidth == 0 || bitmapHeight == 0) {
                glyphs[glyphKey(codepoint, size)] = glyphInfo;
                return;
            }

            PendingGlyph pendingGlyph;
            pendingGlyph.codepoint    = codepoint;
            pendingGlyph.size         = size;
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

        for (char32_t codepoint = kFirstGlyph; codepoint <= kLastGlyph;
             codepoint++) {
            rasterizeGlyph(codepoint);
        }
        for (const char32_t codepoint : kSupplementalGlyphs) {
            rasterizeGlyph(codepoint);
        }

        hb_font_t* hbFont = hb_ft_font_create_referenced(ftFace);
        hbFonts[size]     = hbFont;
    }

    textureWidth  = kAtlasSize;
    textureHeight = kAtlasSize;
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
            SPONGE_CORE_WARN("Glyph 0x{:x} size {} did not fit in atlas",
                             static_cast<uint32_t>(pendingGlyph.codepoint),
                             pendingGlyph.size);
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

        glyphs[glyphKey(pendingGlyph.codepoint, pendingGlyph.size)] = glyphInfo;
    }
}

std::vector<ShapedGlyph> FontAtlas::shape(const std::string_view text,
                                          const uint32_t         size) {
    std::vector<ShapedGlyph> result;

    const auto fontIter = hbFonts.find(size);
    if (fontIter == hbFonts.end()) {
        return result;
    }

    hb_font_t* hbFont = fontIter->second;
    FT_Set_Pixel_Sizes(ftFace, 0, size);
    hb_ft_font_changed(hbFont);

    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, text.data(), static_cast<int>(text.size()), 0,
                       static_cast<int>(text.size()));
    hb_buffer_guess_segment_properties(buffer);

    const hb_feature_t features[] = {
        { HB_TAG('k', 'e', 'r', 'n'), 1, 0, UINT_MAX },
        { HB_TAG('l', 'i', 'g', 'a'), 0, 0, UINT_MAX },
        { HB_TAG('c', 'l', 'i', 'g'), 0, 0, UINT_MAX },
    };
    hb_shape(hbFont, buffer, features, 3);

    unsigned int     glyphCount = 0;
    hb_glyph_info_t* glyphInfos =
        hb_buffer_get_glyph_infos(buffer, &glyphCount);
    hb_glyph_position_t* positions =
        hb_buffer_get_glyph_positions(buffer, &glyphCount);

    result.reserve(glyphCount);
    for (unsigned int index = 0; index < glyphCount; index++) {
        const uint32_t      cluster = glyphInfos[index].cluster;
        const unsigned char byte0   = static_cast<unsigned char>(text[cluster]);
        char32_t            codepoint;
        if (byte0 < 0x80) {
            codepoint = byte0;
        } else if (byte0 < 0xE0) {
            codepoint = static_cast<char32_t>((byte0 & 0x1Fu) << 6u) |
                        (static_cast<unsigned char>(text[cluster + 1]) & 0x3Fu);
        } else {
            codepoint = byte0;
        }

        ShapedGlyph shapedGlyph{};
        shapedGlyph.glyphInfo = getGlyph(codepoint, size);
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

const GlyphInfo* FontAtlas::getGlyph(const char32_t codepoint,
                                     const uint32_t size) const {
    const auto iter = glyphs.find(glyphKey(codepoint, size));
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
