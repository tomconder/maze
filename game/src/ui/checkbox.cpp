#include "ui/checkbox.hpp"

#include "ui/rowlayout.hpp"
#include "ui/rowparts.hpp"

#include <memory>

namespace game::ui {

namespace {
constexpr int boxPart = checkboxparts::box;
}  // namespace

Checkbox::Checkbox(const CheckboxCreateInfo& createInfo) :
    cornerRadius(createInfo.cornerRadius),
    borderWidth(createInfo.borderWidth),
    checkedFill(createInfo.checkedFill),
    checkedBorder(createInfo.checkedBorder),
    uncheckedFill(createInfo.uncheckedFill),
    uncheckedBorder(createInfo.uncheckedBorder),
    quad(std::make_unique<sponge::platform::opengl::scene::Quad>()) {}

void Checkbox::attach(const YGNodeRef row) {
    rowNode = row;
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
