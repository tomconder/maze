#include "ui/selectlist.hpp"

#include <yoga/Yoga.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ui/rowparts.hpp"

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
    attachRowParts(row, partCount, textMarginLeft);
    sizeNodes();
}

void SelectList::sizeNodes() const {
    if (rowNode == nullptr) {
        return;
    }
    const auto textHeight = static_cast<float>(font->getHeight(fontSize));
    setPartSize(rowNode, labelPart, YGUndefined, textHeight);
    setPartSize(rowNode, leftArrowPart, leftLen, textHeight);
    setPartSize(rowNode, valuePart, maxValueWidth, textHeight);
    setPartSize(rowNode, rightArrowPart, rightLen, textHeight);
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
    const auto part = [&](const int index) {
        return partRect(rowNode, index, y);
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
