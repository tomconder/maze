#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace sponge::scene {

struct GlyphInfo {
    float uvLeft, uvTop;  // top-left UV in atlas (0..1), texture-space origin
                          // (row 0 = top of buffer)
    float uvWidth, uvHeight;  // UV extent
    int bearingX;  // horizontal bearing in pixels (pen to left edge of bitmap)
    int bearingY;  // vertical bearing in pixels (baseline to top of bitmap)
    int width;     // bitmap width in pixels
    int height;    // bitmap height in pixels
    float advanceX;  // horizontal advance in pixels
};

struct FontFaceSpec {
    std::string path;  // absolute path to .ttf file
};

class FontAtlas {
public:
    void build(const std::vector<FontFaceSpec>& faces,
               const std::vector<uint32_t>&     sizes);

    const GlyphInfo* getGlyph(char32_t codepoint, uint32_t size) const;
    float getKerning(char32_t left, char32_t right, uint32_t size) const;
    float getLineHeight(uint32_t size) const;
    float getAscender(uint32_t size) const;

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
    static uint64_t kerningKey(const char32_t left, const char32_t right,
                               const uint32_t size) {
        assert(left <= 0xFFFF && right <= 0xFFFF &&
               "kerningKey: codepoints must fit in 16 bits");
        return (static_cast<uint64_t>(size) << 32) |
               (static_cast<uint64_t>(left) << 16) |
               static_cast<uint64_t>(right);
    }

    std::unordered_map<uint64_t, GlyphInfo> glyphs;
    std::unordered_map<uint64_t, float>     kerningMap;
    std::unordered_map<uint32_t, float>     lineHeights;
    std::unordered_map<uint32_t, float>     ascenders;

    std::vector<uint8_t> atlasBuffer;
    uint32_t             textureWidth  = 0;
    uint32_t             textureHeight = 0;
};

}  // namespace sponge::scene
