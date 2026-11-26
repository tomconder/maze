#pragma once

#include "scene/format/font.hpp"
#include "ui/layouttypes.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace game::capture {

// Text metrics read from the baked font file with no window or GL context,
// for tools that print a layout before a window exists. Every size is 0 when
// the file cannot be read.
class FontMetrics {
public:
    // `assetName` is relative to the asset root.
    explicit FontMetrics(std::string_view assetName);

    uint32_t length(std::string_view text, uint32_t size,
                    bool tabularFigures = false);

    uint32_t height(uint32_t size) const;

    // Valid while this object lives.
    ui::TextMeasure measure();

private:
    sponge::scene::font::Font                     font;
    std::vector<sponge::scene::font::ShapedGlyph> shaped;
};

}  // namespace game::capture
