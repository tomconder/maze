#include "ui/checkbox.hpp"

#include "ui/rowparts.hpp"

#include <memory>

namespace game::ui {

namespace {
constexpr int boxPart   = 1;
constexpr int partCount = 2;
}  // namespace

Checkbox::Checkbox(const CheckboxCreateInfo& createInfo) :
    margin(createInfo.margin),
    size(createInfo.size),
    cornerRadius(createInfo.cornerRadius),
    borderWidth(createInfo.borderWidth),
    checkedFill(createInfo.checkedFill),
    checkedBorder(createInfo.checkedBorder),
    uncheckedFill(createInfo.uncheckedFill),
    uncheckedBorder(createInfo.uncheckedBorder),
    quad(std::make_unique<sponge::platform::opengl::scene::Quad>()) {}

void Checkbox::attach(const YGNodeRef row) {
    rowNode = row;
    attachRowParts(row, partCount, margin);
    setSize(size);
}

void Checkbox::setSize(const float newSize) {
    size = newSize;
    if (rowNode != nullptr) {
        setPartSize(rowNode, boxPart, size, size);
    }
}

void Checkbox::onUpdate(const float y, const bool checked) {
    const auto box = partRect(rowNode, boxPart, y);
    quad->render({ box.x, box.y }, { box.x + box.width, box.y + box.height },
                 checked ? checkedFill : uncheckedFill, cornerRadius,
                 borderWidth, checked ? checkedBorder : uncheckedBorder);
}

bool Checkbox::isInside(const float mouseX, const float mouseY,
                        const float y) const {
    const auto box = partRect(rowNode, boxPart, y);
    return mouseX >= box.x && mouseX <= box.x + box.width && mouseY >= box.y &&
           mouseY <= box.y + box.height;
}

}  // namespace game::ui
