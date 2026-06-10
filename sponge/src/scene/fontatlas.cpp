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
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0) {
        SPONGE_CORE_ERROR("FreeType init failed");
        return;
    }

    struct PendingGlyph {
        char32_t             c;
        uint32_t             size;
        GlyphInfo            gi;
        std::vector<uint8_t> bitmap;
        int                  bitmapW;
        int                  bitmapH;
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

            for (char32_t c = kFirstGlyph; c <= kLastGlyph; ++c) {
                if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0) {
                    continue;
                }

                const FT_GlyphSlot g = face->glyph;
                const int          w = static_cast<int>(g->bitmap.width);
                const int          h = static_cast<int>(g->bitmap.rows);

                GlyphInfo gi{};
                gi.bearingX = g->bitmap_left;
                gi.bearingY = g->bitmap_top;
                gi.width    = w;
                gi.height   = h;
                gi.advanceX = static_cast<float>(g->advance.x >> 6);

                if (w == 0 || h == 0) {
                    glyphs[glyphKey(c, size)] = gi;
                    continue;
                }

                PendingGlyph pg;
                pg.c       = c;
                pg.size    = size;
                pg.gi      = gi;
                pg.bitmapW = w;
                pg.bitmapH = h;
                pg.bitmap.assign(g->bitmap.buffer, g->bitmap.buffer + w * h);
                pg.rectIdx = static_cast<int>(rects.size());
                pending.push_back(std::move(pg));

                stbrp_rect r{};
                r.id = pg.rectIdx;
                r.w  = static_cast<stbrp_coord>(w + 1);
                r.h  = static_cast<stbrp_coord>(h + 1);
                rects.push_back(r);
            }

            // Pre-cache kerning for all ASCII pairs at this size
            for (char32_t l = kFirstGlyph; l <= kLastGlyph; ++l) {
                const FT_UInt li = FT_Get_Char_Index(face, l);
                for (char32_t r = kFirstGlyph; r <= kLastGlyph; ++r) {
                    const FT_UInt ri = FT_Get_Char_Index(face, r);
                    FT_Vector     kern{};
                    if (FT_Get_Kerning(face, li, ri, FT_KERNING_DEFAULT,
                                       &kern) == 0 &&
                        kern.x != 0) {
                        kerningMap[kerningKey(l, r, size)] =
                            static_cast<float>(kern.x >> 6);
                    }
                }
            }
        }

        FT_Done_Face(face);
    }

    FT_Done_FreeType(library);

    atlasW = kAtlasSize;
    atlasH = kAtlasSize;
    atlasBuffer.assign(atlasW * atlasH, 0);

    std::vector<stbrp_node> nodes(atlasW);
    stbrp_context           ctx{};
    stbrp_init_target(&ctx, static_cast<int>(atlasW), static_cast<int>(atlasH),
                      nodes.data(), static_cast<int>(nodes.size()));
    stbrp_pack_rects(&ctx, rects.data(), static_cast<int>(rects.size()));

    for (const auto& pg : pending) {
        const auto& rect = rects[pg.rectIdx];
        GlyphInfo   gi   = pg.gi;

        if (!rect.was_packed) {
            SPONGE_CORE_WARN("Glyph 0x{:x} size {} did not fit in atlas",
                             static_cast<uint32_t>(pg.c), pg.size);
            glyphs[glyphKey(pg.c, pg.size)] = gi;
            continue;
        }

        for (int row = 0; row < pg.bitmapH; ++row) {
            std::copy(pg.bitmap.begin() + row * pg.bitmapW,
                      pg.bitmap.begin() + (row + 1) * pg.bitmapW,
                      atlasBuffer.begin() +
                          (rect.y + row) * static_cast<int>(atlasW) + rect.x);
        }

        gi.u   = static_cast<float>(rect.x) / static_cast<float>(atlasW);
        gi.v   = static_cast<float>(rect.y) / static_cast<float>(atlasH);
        gi.uvW = static_cast<float>(pg.bitmapW) / static_cast<float>(atlasW);
        gi.uvH = static_cast<float>(pg.bitmapH) / static_cast<float>(atlasH);

        glyphs[glyphKey(pg.c, pg.size)] = gi;
    }
}

const GlyphInfo* FontAtlas::getGlyph(const char32_t c,
                                     const uint32_t size) const {
    const auto it = glyphs.find(glyphKey(c, size));
    return it != glyphs.end() ? &it->second : nullptr;
}

float FontAtlas::getKerning(const char32_t left, const char32_t right,
                            const uint32_t size) const {
    const auto it = kerningMap.find(kerningKey(left, right, size));
    return it != kerningMap.end() ? it->second : 0.0F;
}

float FontAtlas::getLineHeight(const uint32_t size) const {
    const auto it = lineHeights.find(size);
    return it != lineHeights.end() ? it->second : static_cast<float>(size);
}

float FontAtlas::getAscender(const uint32_t size) const {
    const auto it = ascenders.find(size);
    return it != ascenders.end() ? it->second : static_cast<float>(size);
}

}  // namespace sponge::scene
