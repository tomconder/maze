#pragma once

#include <yoga/Yoga.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "ui/layouttypes.hpp"
#include "ui/menulayout.hpp"
#include "ui/rowparts.hpp"

namespace game::ui {

// The Yoga nodes of one row of a menu, by kind. A function builds the nodes
// the first time it runs on a row and sizes them every time, from the font
// size and a text measure, so the layers and the layout dump share it and
// neither needs a window. The widgets only draw and hit-test: they read where
// the parts are with partRect() and the indexes below.

// A text measure and the font size to measure at.
struct RowText {
    const TextMeasure& measure;
    uint32_t           fontSize;
};

// The space left and right of the parts of a row.
inline constexpr float rowMargin = 26.F;

// The slider sizes are pixels at this menu font size; the slider scales them
// with its font size.
inline constexpr float referenceFontSize = 32.F;

// The radius of a dot under a select list value.
float dotRadius(uint32_t fontSize);

// A heading is a label alone: the first part, labelPart.
void layoutHeading(YGNodeRef row, const RowText& text);

// Label, left arrow, value (a fixed width, so the arrows stay put), right
// arrow. The value part is as tall as the row: the text sits in the middle and
// a strip of dots, one node per item, hangs from the bottom edge.
namespace selectparts {
inline constexpr int leftArrow  = 1;
inline constexpr int value      = 2;
inline constexpr int rightArrow = 3;
// The children of the value part.
inline constexpr int text = 0;
inline constexpr int dots = 1;
}  // namespace selectparts
void layoutSelect(YGNodeRef row, const RowText& text, float maxValueWidth,
                  size_t dotCount);

// Label, left arrow, track, right arrow, value.
namespace sliderparts {
inline constexpr int leftArrow  = 1;
inline constexpr int track      = 2;
inline constexpr int rightArrow = 3;
inline constexpr int value      = 4;
}  // namespace sliderparts
// labelIndent is the space left of the label, in pixels at the reference font
// size, for a row that belongs to the one above it.
void layoutSlider(YGNodeRef row, const RowText& text, float labelIndent = 0.F);

// Label, then a box of boxSize.
namespace checkboxparts {
inline constexpr int box = 1;
}
void layoutCheckbox(YGNodeRef row, const RowText& text, float boxSize);

// Label, then a value part with no width: it marks the right edge, and the
// value text hangs left of it, so a longer text needs no layout.
namespace bindingparts {
inline constexpr int value = 1;
}
void layoutBinding(YGNodeRef row, const RowText& text);

// The node is the button's box; the text is its only child, the first part.
void layoutButton(YGNodeRef box, const RowText& text, std::string_view label,
                  bool centered);

// A column of left-aligned buttons, one to a row of the menu.
void layoutMenuButtons(const MenuLayout& layout, const RowText& text,
                       std::span<const std::string_view> labels);

}  // namespace game::ui
