#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace game::ui {

// The size of a text in pixels.
struct TextSize {
    float width;
    float height;
};

struct Rect {
    float x;
    float y;
    float width;
    float height;
};

// Measures a text at a font size in pixels.
using TextMeasure = std::function<TextSize(std::string_view, uint32_t)>;

}  // namespace game::ui
