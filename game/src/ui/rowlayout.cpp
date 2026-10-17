#include "ui/rowlayout.hpp"

#include <yoga/Yoga.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "ui/rowparts.hpp"

namespace game::ui {

namespace {
constexpr float baseValueGap = 12.F;
// The dots hang this far above the bottom edge of the value part.
constexpr float            dotMargin   = 9.F;
constexpr std::string_view fullPercent = "100%";

float textHeight(const RowText& text) {
    return text.measure("", text.fontSize).height;
}

float textWidth(const RowText& text, const std::string_view string,
                const bool tabularFigures = false) {
    return text.measure(string, text.fontSize, tabularFigures).width;
}

bool isNew(const YGNodeRef row) {
    return YGNodeGetChildCount(row) == 0;
}
}  // namespace

float dotRadius(const uint32_t fontSize) {
    return std::max(3.F, std::round(static_cast<float>(fontSize) / 12.F));
}

void layoutHeading(const YGNodeRef row, const RowText& text,
                   const float margin) {
    if (isNew(row)) {
        attachRowParts(row, 1, margin);
    }
    setPartSize(row, labelPart, YGUndefined, textHeight(text));
}

void layoutSelect(const YGNodeRef row, const RowText& text, const float margin,
                  const float maxValueWidth, const size_t dotCount) {
    if (isNew(row)) {
        attachRowParts(row, 4, margin);

        auto* const valueNode = YGNodeGetChild(row, selectparts::value);
        YGNodeStyleSetAlignSelf(valueNode, YGAlignStretch);
        YGNodeStyleSetJustifyContent(valueNode, YGJustifyCenter);
        YGNodeStyleSetAlignItems(valueNode, YGAlignCenter);
        YGNodeInsertChild(valueNode, YGNodeNew(), selectparts::text);

        auto* const dotsNode = YGNodeNew();
        YGNodeStyleSetPositionType(dotsNode, YGPositionTypeAbsolute);
        YGNodeStyleSetPosition(dotsNode, YGEdgeLeft, 0.F);
        YGNodeStyleSetPosition(dotsNode, YGEdgeRight, 0.F);
        YGNodeStyleSetPosition(dotsNode, YGEdgeBottom, dotMargin);
        YGNodeStyleSetFlexDirection(dotsNode, YGFlexDirectionRow);
        YGNodeStyleSetJustifyContent(dotsNode, YGJustifyCenter);
        YGNodeInsertChild(valueNode, dotsNode, selectparts::dots);
    }

    const auto height = textHeight(text);
    setPartSize(row, labelPart, YGUndefined, height);
    setPartSize(row, selectparts::leftArrow, textWidth(text, "< "), height);
    setPartSize(row, selectparts::rightArrow, textWidth(text, " >"), height);

    auto* const valueNode = YGNodeGetChild(row, selectparts::value);
    YGNodeStyleSetWidth(valueNode, maxValueWidth);
    setPartSize(valueNode, selectparts::text, maxValueWidth, height);

    auto* const dotsNode = YGNodeGetChild(valueNode, selectparts::dots);
    while (YGNodeGetChildCount(dotsNode) > dotCount) {
        auto* const dot =
            YGNodeGetChild(dotsNode, YGNodeGetChildCount(dotsNode) - 1);
        YGNodeRemoveChild(dotsNode, dot);
        YGNodeFree(dot);
    }
    while (YGNodeGetChildCount(dotsNode) < dotCount) {
        YGNodeInsertChild(dotsNode, YGNodeNew(), YGNodeGetChildCount(dotsNode));
    }
    const auto radius = dotRadius(text.fontSize);
    YGNodeStyleSetGap(dotsNode, YGGutterColumn, radius * 1.5F);
    for (size_t i = 0; i < dotCount; i++) {
        setPartSize(dotsNode, static_cast<int>(i), radius * 2.F, radius * 2.F);
    }
}

void layoutSlider(const YGNodeRef row, const RowText& text, const float margin,
                  const SliderSizes& sizes) {
    if (isNew(row)) {
        attachRowParts(row, 5, margin);
    }
    const auto scale = static_cast<float>(text.fontSize) / referenceFontSize;
    YGNodeStyleSetGap(row, YGGutterColumn, baseValueGap * scale);

    const auto height = textHeight(text);
    setPartSize(row, labelPart, YGUndefined, height);
    YGNodeStyleSetMargin(YGNodeGetChild(row, labelPart), YGEdgeLeft,
                         sizes.labelIndent * scale);
    setPartSize(row, sliderparts::leftArrow, textWidth(text, "< "), height);
    setPartSize(row, sliderparts::track, sizes.trackWidth * scale,
                sizes.trackHeight * scale);
    setPartSize(row, sliderparts::rightArrow, textWidth(text, " >"), height);
    setPartSize(row, sliderparts::value, textWidth(text, fullPercent, true),
                height);
}

void layoutCheckbox(const YGNodeRef row, const RowText& text,
                    const float margin, const float boxSize) {
    if (isNew(row)) {
        attachRowParts(row, 2, margin);
    }
    setPartSize(row, labelPart, YGUndefined, textHeight(text));
    setPartSize(row, checkboxparts::box, boxSize, boxSize);
}

void layoutBinding(const YGNodeRef row, const RowText& text,
                   const float margin) {
    if (isNew(row)) {
        attachRowParts(row, 2, margin);
    }
    const auto height = textHeight(text);
    setPartSize(row, labelPart, YGUndefined, height);
    setPartSize(row, bindingparts::value, 0.F, height);
}

void layoutButton(const YGNodeRef box, const RowText& text, const float margin,
                  const std::string_view label, const bool centered) {
    if (isNew(box)) {
        YGNodeStyleSetFlexDirection(box, YGFlexDirectionRow);
        YGNodeStyleSetAlignItems(box, YGAlignCenter);
        YGNodeStyleSetPadding(box, YGEdgeHorizontal, margin);
        YGNodeInsertChild(box, YGNodeNew(), labelPart);
    }
    YGNodeStyleSetJustifyContent(box, centered ? YGJustifyCenter :
                                                 YGJustifyFlexStart);
    // The text is centred on the font size, not the line height.
    setPartSize(box, labelPart, textWidth(text, label),
                static_cast<float>(text.fontSize));
}

void layoutMenuButtons(const MenuLayout& layout, const RowText& text,
                       const std::span<const std::string_view> labels) {
    for (size_t row = 0; row < labels.size(); row++) {
        layoutButton(layout.rowNode(row), text, rowMargin, labels[row], false);
    }
}

}  // namespace game::ui
