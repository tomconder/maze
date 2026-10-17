#pragma once

#include "font.hpp"
#include "ui/layouttypes.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace game::ui {

// Text metrics read from the baked font file with no window or GL context,
// for tools that print a layout before a window exists. Every size is 0 when
// the file cannot be read.
class FontMetrics {
public:
    explicit FontMetrics(const std::string& path);

    uint32_t length(std::string_view text, uint32_t size,
                    bool tabularFigures = false);

    uint32_t height(uint32_t size) const;

    // Valid while this object lives.
    TextMeasure measure();

private:
    sponge::scene::font::Font                     font;
    std::vector<sponge::scene::font::ShapedGlyph> shaped;
};

}  // namespace game::ui
