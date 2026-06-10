#include "scene/fontatlas.hpp"

#include "logging/log.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stb_rect_pack.h>

#include <algorithm>
#include <cassert>
#include <vector>

namespace sponge::scene {

namespace {
constexpr uint32_t kAtlasSize  = 512;
constexpr char32_t kFirstGlyph = 32;
constexpr char32_t kLastGlyph  = 126;
}  // namespace

void FontAtlas::build(const std::vector<FontFaceSpec>& faces,
                      const std::vector<uint32_t>&     sizes) {
    assert(textureWidth == 0 && "FontAtlas::build() called more than once");
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0) {
        SPONGE_CORE_ERROR("FreeType init failed");
        return;
    }
    struct LibraryGuard {
        FT_Library& lib;
        ~LibraryGuard() {
            FT_Done_FreeType(lib);
        }
    } libraryGuard{ library };

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

    for (const auto& faceSpec : faces) {
        FT_Face face = nullptr;
        if (FT_New_Face(library, faceSpec.path.c_str(), 0, &face) != 0) {
            SPONGE_CORE_ERROR("Failed to load font: {}", faceSpec.path);
            continue;
        }

        for (const uint32_t size : sizes) {
            FT_Set_Pixel_Sizes(face, 0, size);

            lineHeights[size] =
                static_cast<float>(face->size->metrics.height >> 6);
            ascenders[size] =
                static_cast<float>(face->size->metrics.ascender >> 6);

            for (char32_t codepoint = kFirstGlyph; codepoint <= kLastGlyph;
                 codepoint++) {
                if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER) != 0) {
                    continue;
                }

                const FT_GlyphSlot glyphSlot = face->glyph;
                const int          bitmapWidth =
                    static_cast<int>(glyphSlot->bitmap.width);
                const int bitmapHeight =
                    static_cast<int>(glyphSlot->bitmap.rows);

                GlyphInfo glyphInfo{};
                glyphInfo.bearingX = glyphSlot->bitmap_left;
                glyphInfo.bearingY = glyphSlot->bitmap_top;
                glyphInfo.width    = bitmapWidth;
                glyphInfo.height   = bitmapHeight;
                glyphInfo.advanceX =
                    static_cast<float>(glyphSlot->advance.x >> 6);

                if (bitmapWidth == 0 || bitmapHeight == 0) {
                    glyphs[glyphKey(codepoint, size)] = glyphInfo;
                    continue;
                }

                PendingGlyph pendingGlyph;
                pendingGlyph.codepoint    = codepoint;
                pendingGlyph.size         = size;
                pendingGlyph.glyphInfo    = glyphInfo;
                pendingGlyph.bitmapWidth  = bitmapWidth;
                pendingGlyph.bitmapHeight = bitmapHeight;
                pendingGlyph.bitmap.assign(glyphSlot->bitmap.buffer,
                                           glyphSlot->bitmap.buffer +
                                               bitmapWidth * bitmapHeight);
                pendingGlyph.rectIdx = static_cast<int>(rects.size());
                pending.push_back(std::move(pendingGlyph));

                stbrp_rect rect{};
                rect.id = pending.back().rectIdx;
                rect.w  = static_cast<stbrp_coord>(bitmapWidth + 1);
                rect.h  = static_cast<stbrp_coord>(bitmapHeight + 1);
                rects.push_back(rect);
            }

            // Pre-cache kerning for all ASCII pairs at this size
            for (char32_t leftChar = kFirstGlyph; leftChar <= kLastGlyph;
                 leftChar++) {
                const FT_UInt leftIndex = FT_Get_Char_Index(face, leftChar);
                for (char32_t rightChar = kFirstGlyph; rightChar <= kLastGlyph;
                     rightChar++) {
                    const FT_UInt rightIndex =
                        FT_Get_Char_Index(face, rightChar);
                    FT_Vector kern{};
                    if (FT_Get_Kerning(face, leftIndex, rightIndex,
                                       FT_KERNING_DEFAULT, &kern) == 0 &&
                        kern.x != 0) {
                        kerningMap[kerningKey(leftChar, rightChar, size)] =
                            static_cast<float>(kern.x >> 6);
                    }
                }
            }
        }

        FT_Done_Face(face);
    }

    textureWidth  = kAtlasSize;
    textureHeight = kAtlasSize;
    atlasBuffer.assign(textureWidth * textureHeight, 0);

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
                          row * pendingGlyph.bitmapWidth,
                      pendingGlyph.bitmap.begin() +
                          (row + 1) * pendingGlyph.bitmapWidth,
                      atlasBuffer.begin() +
                          static_cast<ptrdiff_t>(rect.y + row) *
                              static_cast<ptrdiff_t>(textureWidth) +
                          rect.x);
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

const GlyphInfo* FontAtlas::getGlyph(const char32_t codepoint,
                                     const uint32_t size) const {
    const auto iter = glyphs.find(glyphKey(codepoint, size));
    return iter != glyphs.end() ? &iter->second : nullptr;
}

float FontAtlas::getKerning(const char32_t left, const char32_t right,
                            const uint32_t size) const {
    const auto iter = kerningMap.find(kerningKey(left, right, size));
    return iter != kerningMap.end() ? iter->second : 0.0F;
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
