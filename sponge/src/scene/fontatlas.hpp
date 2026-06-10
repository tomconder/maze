#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// clang-format off
#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
// clang-format on

namespace sponge::scene {

struct GlyphInfo {
    float uvLeft, uvTop;      // top-left UV in atlas (0..1)
    float uvWidth, uvHeight;  // UV extent
    int   bearingX;           // horizontal bearing in pixels
    int   bearingY;           // vertical bearing in pixels
    int   width;              // bitmap width in pixels
    int   height;             // bitmap height in pixels
    float advanceX;           // horizontal advance in pixels
};

struct FontFaceSpec {
    std::string path;  // absolute path to .ttf file
};

struct ShapedGlyph {
    const GlyphInfo* glyphInfo;  // nullptr if not in atlas
    float            xAdvance;   // horizontal advance (pixels)
    float            xOffset;    // horizontal offset (pixels)
    float            yOffset;    // vertical offset (pixels)
};

class FontAtlas {
public:
    ~FontAtlas();

    void build(const std::vector<FontFaceSpec>& faces,
               const std::vector<uint32_t>&     sizes);

    std::vector<ShapedGlyph> shape(std::string_view text, uint32_t size);

    const GlyphInfo* getGlyph(char32_t codepoint, uint32_t size) const;
    float            getLineHeight(uint32_t size) const;
    float            getAscender(uint32_t size) const;

    const uint8_t* data() const {
        return atlasBuffer.data();
    }
    uint32_t atlasWidth() const {
        return textureWidth;
    }
    uint32_t atlasHeight() const {
        return textureHeight;
    }

private:
    static uint64_t glyphKey(const char32_t codepoint, const uint32_t size) {
        return (static_cast<uint64_t>(codepoint) << 32) | size;
    }

    std::unordered_map<uint64_t, GlyphInfo>  glyphs;
    std::unordered_map<uint32_t, float>      lineHeights;
    std::unordered_map<uint32_t, float>      ascenders;
    std::unordered_map<uint32_t, hb_font_t*> hbFonts;

    std::vector<uint8_t> atlasBuffer;
    uint32_t             textureWidth  = 0;
    uint32_t             textureHeight = 0;

    FT_Library ftLibrary = nullptr;
    FT_Face    ftFace    = nullptr;
};

}  // namespace sponge::scene
