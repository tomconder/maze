#pragma once

#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Baked font tables. A font bakes to one KTX2 file: the glyph atlas as its
// image, and these tables under the fontKey key/value entry. Written by
// tools/assetconv, read by BitmapFont.
//
// Shaping is table lookup. The converter shapes with HarfBuzz with ligatures
// and contextual alternates off, and records what it gets: a glyph per
// codepoint, an advance per glyph and a kerning adjustment per glyph pair. It
// then checks that shape() below matches HarfBuzz, so a font that needs more
// than pair kerning fails the bake.
namespace sponge::scene::font {

constexpr std::string_view fontKey = "spongeFont";
// Increase when the layout or subpixelPhases changes.
constexpr uint32_t version = 1;

// Horizontal subpixel positions baked per glyph, in 1/4 pixel steps.
constexpr uint32_t subpixelPhases = 4;

// Atlas placement of one glyph at one size and phase. width and height of 0
// mean nothing to draw.
struct Glyph {
    float   uvLeft;
    float   uvTop;
    float   uvWidth;
    float   uvHeight;
    int32_t bearingX;
    int32_t bearingY;
    int32_t width;
    int32_t height;
};

// Advances and kerning are 26.6 fixed point, as HarfBuzz reports them.
struct Size {
    uint32_t             pixels{ 0 };
    float                lineHeight{ 0.F };
    float                ascender{ 0.F };
    std::vector<int32_t> advances;  // per slot
    std::vector<Glyph>   glyphs;    // slot * subpixelPhases + phase
    // (first slot << 16 | second slot) to the change in the first advance.
    std::map<uint32_t, int32_t> kerning;
};

// Glyphs are numbered by slot, not by font glyph index. Slot 0 is the
// missing glyph: an unknown codepoint advances by its width and draws
// nothing.
struct Font {
    uint32_t                     slotCount{ 0 };
    std::map<char32_t, uint32_t> slots;
    std::map<char32_t, uint32_t> tabularSlots;  // with fixed-width digits
    std::vector<Size>            sizes;
};

struct ShapedGlyph {
    uint32_t slot;
    float    xAdvance;  // pixels
};

std::vector<uint8_t> write(const Font& font);

// On failure, returns a Font with no sizes and sets error. error is left
// alone on success, so pass an empty string.
Font read(std::span<const uint8_t> bytes, std::string& error);

// Returns nullptr when the font was not baked at this size.
const Size* sizeOf(const Font& font, uint32_t pixels);

// UTF-8 text to glyphs. tabularFigures selects fixed-width digits (OpenType
// "tnum").
std::vector<ShapedGlyph> shape(const Font& font, const Size& size,
                               std::string_view text, bool tabularFigures);

}  // namespace sponge::scene::font
