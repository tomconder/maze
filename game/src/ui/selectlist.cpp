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

#include "ui/rowlayout.hpp"
#include "ui/rowparts.hpp"

namespace game::ui {

namespace {
constexpr std::string_view leftArrow  = "< ";
constexpr std::string_view rightArrow = " >";

constexpr int leftArrowPart  = selectparts::leftArrow;
constexpr int valuePart      = selectparts::value;
constexpr int rightArrowPart = selectparts::rightArrow;
constexpr int textNode       = selectparts::text;
constexpr int dotsNode       = selectparts::dots;

constexpr glm::vec4 dotColorDefault = { 0.25F, 0.25F, 0.25F, 1.F };
constexpr glm::vec4 dotColorCurrent = { 1.F, 1.F, 1.F, 1.F };
constexpr glm::vec4 dotColorDisplay = { 0.65F, 0.65F, 0.65F, 1.F };
}  // namespace

SelectList::SelectList(const SelectListCreateInfo& createInfo) :
    font(createInfo.font),
    fontSize(createInfo.fontSize),
    textColor(createInfo.textColor),
    arrowDisabledColor(createInfo.arrowDisabledColor),
    tabularFigures(createInfo.tabularFigures),
    quad(std::make_unique<sponge::platform::opengl::scene::Quad>()) {}

void SelectList::setItems(std::vector<std::string> newItems) {
    items = std::move(newItems);
    if (selectedIndex >= items.size()) {
        selectedIndex = 0;
    }
}

void SelectList::attach(const YGNodeRef row) {
    rowNode = row;
}

void SelectList::setAppliedIndex(const std::optional<size_t> index) {
    appliedIndex = index;
}

void SelectList::setFontSize(const uint32_t size) {
    fontSize = size;
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
                     dotRadius(fontSize));
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
