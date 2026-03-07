#include "ui/checkbox.hpp"

#include <memory>

namespace game::ui {

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

void Checkbox::onUpdate(const float x, const float y, const float w,
                        const float h, const bool checked) {
    const float boxX = x + w - margin - size;
    const float boxY = y + (h - size) / 2.F;
    quad->render({ boxX, boxY }, { boxX + size, boxY + size },
                 checked ? checkedFill : uncheckedFill, cornerRadius,
                 borderWidth, checked ? checkedBorder : uncheckedBorder);
}

bool Checkbox::isInside(const float mouseX, const float mouseY, const float x,
                        const float y, const float w, const float h) const {
    const float boxX = x + w - margin - size;
    const float boxY = y + (h - size) / 2.F;
    return mouseX >= boxX && mouseX <= boxX + size && mouseY >= boxY &&
           mouseY <= boxY + size;
}

}  // namespace game::ui
