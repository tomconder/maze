#include "ui/selectlist.hpp"

#include <yoga/Yoga.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ui/rowparts.hpp"

namespace game::ui {

namespace {
constexpr std::string_view leftArrow  = "< ";
constexpr std::string_view rightArrow = " >";

// The children of the row after the label, left to right.
constexpr int leftArrowPart  = 1;
constexpr int valuePart      = 2;
constexpr int rightArrowPart = 3;
constexpr int partCount      = 4;

// The value part is the text, then the dots, which hang from its bottom edge.
constexpr int   textNode        = 0;
constexpr int   dotsNode        = 1;
constexpr float dotBottomMargin = 9.F;

constexpr glm::vec4 dotColorDefault = { 0.25F, 0.25F, 0.25F, 1.F };
constexpr glm::vec4 dotColorCurrent = { 1.F, 1.F, 1.F, 1.F };
constexpr glm::vec4 dotColorDisplay = { 0.65F, 0.65F, 0.65F, 1.F };
}  // namespace

SelectList::SelectList(const SelectListCreateInfo& createInfo) :
    font(createInfo.font),
    fontSize(createInfo.fontSize),
    textColor(createInfo.textColor),
    arrowDisabledColor(createInfo.arrowDisabledColor),
    textMarginLeft(createInfo.textMarginLeft),
    maxValueWidth(createInfo.maxValueWidth),
    tabularFigures(createInfo.tabularFigures),
    quad(std::make_unique<sponge::platform::opengl::scene::Quad>()) {
    leftLen  = static_cast<float>(font->getLength(leftArrow, fontSize));
    rightLen = static_cast<float>(font->getLength(rightArrow, fontSize));
}

void SelectList::setItems(std::vector<std::string> newItems) {
    items = std::move(newItems);
    if (selectedIndex >= items.size()) {
        selectedIndex = 0;
    }
}

void SelectList::attach(const YGNodeRef row) {
    rowNode = row;
    attachRowParts(row, partCount, textMarginLeft);

    // The value part is as tall as the row, so the dots can hang from its
    // bottom edge while the text stays in the middle.
    auto* const value = YGNodeGetChild(row, valuePart);
    YGNodeStyleSetAlignSelf(value, YGAlignStretch);
    YGNodeStyleSetJustifyContent(value, YGJustifyCenter);
    YGNodeStyleSetAlignItems(value, YGAlignCenter);
    YGNodeInsertChild(value, YGNodeNew(), textNode);

    auto* const dots = YGNodeNew();
    YGNodeStyleSetPositionType(dots, YGPositionTypeAbsolute);
    YGNodeStyleSetPosition(dots, YGEdgeLeft, 0.F);
    YGNodeStyleSetPosition(dots, YGEdgeRight, 0.F);
    YGNodeStyleSetPosition(dots, YGEdgeBottom, dotBottomMargin);
    YGNodeStyleSetFlexDirection(dots, YGFlexDirectionRow);
    YGNodeStyleSetJustifyContent(dots, YGJustifyCenter);
    YGNodeInsertChild(value, dots, dotsNode);

    rebuildDots();
}

void SelectList::sizeNodes() const {
    if (rowNode == nullptr) {
        return;
    }
    const auto textHeight = static_cast<float>(font->getHeight(fontSize));
    setPartSize(rowNode, labelPart, YGUndefined, textHeight);
    setPartSize(rowNode, leftArrowPart, leftLen, textHeight);
    setPartSize(rowNode, rightArrowPart, rightLen, textHeight);

    auto* const value = YGNodeGetChild(rowNode, valuePart);
    YGNodeStyleSetWidth(value, maxValueWidth);
    setPartSize(value, textNode, maxValueWidth, textHeight);

    const auto  diameter = dotRadius() * 2.F;
    auto* const dots     = YGNodeGetChild(value, dotsNode);
    YGNodeStyleSetGap(dots, YGGutterColumn, dotRadius() * 1.5F);
    for (size_t i = 0; i < YGNodeGetChildCount(dots); i++) {
        auto* const dot = YGNodeGetChild(dots, i);
        YGNodeStyleSetWidth(dot, diameter);
        YGNodeStyleSetHeight(dot, diameter);
    }
}

float SelectList::dotRadius() const {
    return std::max(3.F, std::round(static_cast<float>(fontSize) / 12.F));
}

void SelectList::rebuildDots() const {
    auto* const dots =
        YGNodeGetChild(YGNodeGetChild(rowNode, valuePart), dotsNode);
    while (YGNodeGetChildCount(dots) > 0) {
        auto* const dot = YGNodeGetChild(dots, YGNodeGetChildCount(dots) - 1);
        YGNodeRemoveChild(dots, dot);
        YGNodeFree(dot);
    }
    for (size_t i = 0; i < dotCount; i++) {
        YGNodeInsertChild(dots, YGNodeNew(), i);
    }
    sizeNodes();
}

bool SelectList::setDotCount(const size_t count) {
    if (count == dotCount) {
        return false;
    }
    dotCount = count;
    if (rowNode != nullptr) {
        rebuildDots();
    }
    return true;
}

void SelectList::setAppliedIndex(const std::optional<size_t> index) {
    appliedIndex = index;
}

void SelectList::setMaxValueWidth(const float width) {
    maxValueWidth = width;
    sizeNodes();
}

void SelectList::setFontSize(const uint32_t size) {
    if (fontSize == size) {
        return;
    }
    fontSize = size;
    leftLen  = static_cast<float>(font->getLength(leftArrow, fontSize));
    rightLen = static_cast<float>(font->getLength(rightArrow, fontSize));
    sizeNodes();
}

size_t SelectList::getSelectedIndex() const {
    return selectedIndex;
}

void SelectList::setSelectedIndex(const size_t index) {
    selectedIndex = index;
}

void SelectList::setDisabled(const bool value) {
    disabled = value;
}

bool SelectList::selectPrev() {
    if (!disabled && selectedIndex > 0) {
        selectedIndex--;
        return true;
    }
    return false;
}

bool SelectList::selectNext() {
    if (!disabled && !items.empty() && selectedIndex + 1 < items.size()) {
        selectedIndex++;
        return true;
    }
    return false;
}

std::string_view SelectList::getValue() const {
    if (items.empty()) {
        return {};
    }
    return items[selectedIndex];
}

bool SelectList::hasLeft() const {
    return !disabled && selectedIndex > 0;
}

bool SelectList::hasRight() const {
    return !disabled && !items.empty() && selectedIndex + 1 < items.size();
}

void SelectList::onUpdate(const float y, const std::string_view label) {
    const auto part = [&](const int index) {
        return partRect(rowNode, index, y);
    };
    const auto labelRect     = part(labelPart);
    const auto leftArrowRect = part(leftArrowPart);
    const auto valueRect =
        rectAt(rowNode,
               YGNodeGetChild(YGNodeGetChild(rowNode, valuePart), textNode), y);
    const auto rightArrowRect = part(rightArrowPart);

    const auto value = getValue();
    const auto valLen =
        static_cast<float>(font->getLength(value, fontSize, tabularFigures));
    const float valueX = valueRect.x + (valueRect.width - valLen) / 2.F;

    const auto& color = disabled ? arrowDisabledColor : textColor;

    font->beginPass(fontSize);
    font->render(label, { labelRect.x, labelRect.y }, color);
    font->render(leftArrow, { leftArrowRect.x, leftArrowRect.y },
                 hasLeft() ? textColor : arrowDisabledColor);
    font->render(value, { valueX, valueRect.y }, color, tabularFigures);
    font->render(rightArrow, { rightArrowRect.x, rightArrowRect.y },
                 hasRight() ? textColor : arrowDisabledColor);
    font->endPass();

    renderDots(y);
}

void SelectList::renderDots(const float y) const {
    auto* const dots =
        YGNodeGetChild(YGNodeGetChild(rowNode, valuePart), dotsNode);
    for (size_t i = 0; i < YGNodeGetChildCount(dots); i++) {
        const auto dot   = rectAt(rowNode, YGNodeGetChild(dots, i), y);
        const auto color = appliedIndex && i == *appliedIndex ?
                               dotColorCurrent :
                           i == selectedIndex ? dotColorDisplay :
                                                dotColorDefault;
        quad->render({ dot.x, dot.y },
                     { dot.x + dot.width, dot.y + dot.height }, color,
                     dotRadius());
    }
}

bool SelectList::isInsideLeft(const float mouseX) const {
    const auto left = nodeRect(YGNodeGetChild(rowNode, leftArrowPart));
    return hasLeft() && mouseX >= left.x && mouseX <= left.x + left.width;
}

bool SelectList::isInsideRight(const float mouseX) const {
    const auto right = nodeRect(YGNodeGetChild(rowNode, rightArrowPart));
    return hasRight() && mouseX >= right.x && mouseX <= right.x + right.width;
}

}  // namespace game::ui
