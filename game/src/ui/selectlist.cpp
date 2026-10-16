#include "ui/selectlist.hpp"

#include <yoga/Yoga.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ui/yogarect.hpp"

namespace game::ui {

namespace {
constexpr std::string_view leftArrow  = "< ";
constexpr std::string_view rightArrow = " >";

// The children of the row, left to right.
constexpr int labelPart      = 0;
constexpr int leftArrowPart  = 1;
constexpr int valuePart      = 2;
constexpr int rightArrowPart = 3;
constexpr int partCount      = 4;
}  // namespace

SelectList::SelectList(const SelectListCreateInfo& createInfo) :
    font(createInfo.font),
    fontSize(createInfo.fontSize),
    textColor(createInfo.textColor),
    arrowDisabledColor(createInfo.arrowDisabledColor),
    textMarginLeft(createInfo.textMarginLeft),
    maxValueWidth(createInfo.maxValueWidth),
    tabularFigures(createInfo.tabularFigures) {
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
    YGNodeStyleSetFlexDirection(row, YGFlexDirectionRow);
    YGNodeStyleSetAlignItems(row, YGAlignCenter);
    YGNodeStyleSetPadding(row, YGEdgeHorizontal, textMarginLeft);
    for (int part = 0; part < partCount; part++) {
        YGNodeInsertChild(row, YGNodeNew(), part);
    }
    YGNodeStyleSetFlexGrow(YGNodeGetChild(row, labelPart), 1.F);
    sizeNodes();
}

void SelectList::sizeNodes() const {
    if (rowNode == nullptr) {
        return;
    }
    const auto textHeight = static_cast<float>(font->getHeight(fontSize));
    const auto setSize    = [&](const int part, const float w, const float h) {
        auto* const node = YGNodeGetChild(rowNode, part);
        YGNodeStyleSetWidth(node, w);
        YGNodeStyleSetHeight(node, h);
    };
    // the label has no width of its own: it takes what the rest leaves
    YGNodeStyleSetHeight(YGNodeGetChild(rowNode, labelPart), textHeight);
    setSize(leftArrowPart, leftLen, textHeight);
    setSize(valuePart, maxValueWidth, textHeight);
    setSize(rightArrowPart, rightLen, textHeight);
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

float SelectList::getValueCenterX() const {
    const auto value = nodeRect(YGNodeGetChild(rowNode, valuePart));
    return value.x + value.width / 2.F;
}

void SelectList::onUpdate(const float y, const std::string_view label) {
    const auto rowTop = nodeRect(rowNode).y;
    const auto part   = [&](const int index) {
        auto rect = nodeRect(YGNodeGetChild(rowNode, index));
        rect.y += y - rowTop;
        return rect;
    };
    const auto labelRect      = part(labelPart);
    const auto leftArrowRect  = part(leftArrowPart);
    const auto valueRect      = part(valuePart);
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
