#include "ui/button.hpp"

#include "core/file.hpp"
#include "platform/audio/audio.hpp"
#include "ui/rowparts.hpp"
#include "ui/yogarect.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace game::ui {

Button::Button(const ButtonCreateInfo& createInfo) :
    text(createInfo.message),
    textSize(createInfo.fontSize),
    color(createInfo.buttonColor),
    textColor(createInfo.textColor),
    cornerRadius(createInfo.cornerRadius),
    borderWidth(createInfo.borderWidth),
    borderColor(createInfo.borderColor) {
    font = createInfo.font;

    quad = std::make_unique<sponge::platform::opengl::scene::Quad>();
}

void Button::attach(const YGNodeRef box) {
    node = box;
}

bool Button::onUpdate(const double) const {
    const auto box      = nodeRect(node);
    const auto textRect = nodeRect(YGNodeGetChild(node, 0));
    quad->render({ box.x, box.y }, { box.x + box.width, box.y + box.height },
                 color, cornerRadius, borderWidth, borderColor);
    font->beginPass(textSize);
    font->render(text, { textRect.x, textRect.y }, textColor);
    font->endPass();

    return true;
}

bool Button::isInside(const glm::vec2& position) const {
    const auto box = nodeRect(node);
    return box.x <= position.x && box.y <= position.y &&
           box.x + box.width >= position.x && box.y + box.height >= position.y;
}

void Button::setButtonColor(const glm::vec4& val) {
    color = glm::vec4(val);
}

void Button::setTextColor(const glm::vec3& val) {
    textColor = glm::vec3(val);
}

void Button::setBorderWidth(const float val) {
    borderWidth = val;
}

void Button::setBorderColor(const glm::vec4& val) {
    borderColor = val;
}

void Button::setFontSize(const uint32_t size) {
    textSize = size;
}

std::unique_ptr<Button> makeMenuButton(
    std::string_view message, const uint32_t fontSize,
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font,
    const glm::vec4& buttonColor, const glm::vec3& textColor) {
    return std::make_unique<Button>(ButtonCreateInfo{
        .message      = std::string(message),
        .fontSize     = fontSize,
        .font         = std::move(font),
        .buttonColor  = buttonColor,
        .textColor    = textColor,
        .cornerRadius = 12.F,
    });
}

void updateMenuButtonVisuals(Button* button, const bool selected,
                             const glm::vec4& selectedColor) {
    constexpr glm::vec4 hoverColor = { 0.84F, 0.84F, 0.84F, 0.14F };

    if (selected) {
        button->setBorderWidth(3.F);
        button->setBorderColor(glm::vec4{ 1.F });
        button->setButtonColor(selectedColor);
    } else if (!button->hasHover()) {
        button->setBorderWidth(0.F);
        button->setButtonColor(glm::vec4{ 0.F });
    } else {
        button->setBorderWidth(0.F);
        button->setButtonColor(hoverColor);
    }
}

void playHoverClick() {
    sponge::platform::audio::Audio::play(sponge::core::File::getResourceDir() +
                                         "/audio/click.wav");
}

void updateButtonHover(Button* button, const glm::vec2& pos) {
    if (!button->hasHover() && button->isInside(pos)) {
        button->setHover(true);
        playHoverClick();
    } else if (button->hasHover() && !button->isInside(pos)) {
        button->setHover(false);
    }
}
}  // namespace game::ui
