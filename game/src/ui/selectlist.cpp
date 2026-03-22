#include "ui/selectlist.hpp"

#include <cmath>
#include <string_view>

namespace game::ui {
using sponge::platform::opengl::renderer::AssetManager;

namespace {
constexpr std::string_view leftArrow  = "< ";
constexpr std::string_view rightArrow = " >";
}  // namespace

SelectList::SelectList(const SelectListCreateInfo& createInfo) :
    font(AssetManager::getFont(createInfo.fontName)),
    fontSize(createInfo.fontSize),
    textColor(createInfo.textColor),
    arrowDisabledColor(createInfo.arrowDisabledColor),
    textMarginLeft(createInfo.textMarginLeft),
    maxValueWidth(createInfo.maxValueWidth) {
    leftLen  = static_cast<float>(font->getLength(leftArrow, fontSize));
    rightLen = static_cast<float>(font->getLength(rightArrow, fontSize));
}

void SelectList::setItems(std::vector<std::string> newItems) {
    items = std::move(newItems);
    if (selectedIndex >= items.size()) {
        selectedIndex = 0;
    }
}

void SelectList::setMaxValueWidth(const float width) {
    maxValueWidth = width;
}

size_t SelectList::getSelectedIndex() const {
    return selectedIndex;
}

void SelectList::setSelectedIndex(const size_t index) {
    selectedIndex = index;
}

bool SelectList::selectPrev() {
    if (selectedIndex > 0) {
        selectedIndex--;
        return true;
    }
    return false;
}

bool SelectList::selectNext() {
    if (!items.empty() && selectedIndex + 1 < items.size()) {
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
    return selectedIndex > 0;
}

bool SelectList::hasRight() const {
    return !items.empty() && selectedIndex + 1 < items.size();
}

float SelectList::getStartX(const float x, const float w) const {
    return x + w - textMarginLeft - leftLen - maxValueWidth - rightLen;
}

void SelectList::onUpdate(const float x, const float y, const float w,
                          const float h, const std::string_view label) {
    const float textY = std::floor(
        y + (h - static_cast<float>(font->getHeight(fontSize))) / 2.F);
    const auto  value  = getValue();
    const auto  valLen = static_cast<float>(font->getLength(value, fontSize));
    const float startX = getStartX(x, w);
    const float valueX = startX + leftLen + (maxValueWidth - valLen) / 2.F;

    font->render(label, { x + textMarginLeft, textY }, fontSize, textColor);
    font->render(leftArrow, { startX, textY }, fontSize,
                 hasLeft() ? textColor : arrowDisabledColor);
    font->render(value, { valueX, textY }, fontSize, textColor);
    font->render(rightArrow, { startX + leftLen + maxValueWidth, textY },
                 fontSize, hasRight() ? textColor : arrowDisabledColor);
}

bool SelectList::isInsideLeft(const float mouseX, const float x,
                              const float w) const {
    const float startX = getStartX(x, w);
    return hasLeft() && mouseX >= startX && mouseX <= startX + leftLen;
}

bool SelectList::isInsideRight(const float mouseX, const float x,
                               const float w) const {
    const float startX = getStartX(x, w);
    return hasRight() && mouseX >= startX + leftLen + maxValueWidth &&
           mouseX <= startX + leftLen + maxValueWidth + rightLen;
}

}  // namespace game::ui
