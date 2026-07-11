#include "ui/button.hpp"

#include <memory>
#include <string_view>

namespace game::ui {

Button::Button(const ButtonCreateInfo& createInfo) :
    top(createInfo.topLeft),
    bottom(createInfo.bottomRight),
    text(createInfo.message),
    textSize(createInfo.fontSize),
    color(createInfo.buttonColor),
    textColor(createInfo.textColor),
    marginLeft(createInfo.marginLeft),
    cornerRadius(createInfo.cornerRadius),
    borderWidth(createInfo.borderWidth),
    borderColor(createInfo.borderColor),
    alignType(createInfo.alignType),
    textPosition({ createInfo.topLeft.x, createInfo.topLeft.y }) {
    font   = createInfo.font;
    length = font->getLength(text, textSize);

    quad = std::make_unique<sponge::platform::opengl::scene::Quad>();
}

bool Button::onUpdate(const double elapsedTime) const {
    UNUSED(elapsedTime);

    quad->render(top, bottom, color, cornerRadius, borderWidth, borderColor);
    font->beginPass(textSize);
    font->render(text, textPosition, textColor);
    font->endPass();

    return true;
}

bool Button::isInside(const glm::vec2& position) const {
    return top.x <= position.x && top.y <= position.y &&
           bottom.x >= position.x && bottom.y >= position.y;
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

void Button::setAlignType(const ButtonAlignType& val) {
    alignType = val;
}

void Button::setMessage(std::string_view message) {
    if (text == message) {
        return;
    }

    text   = message;
    length = font->getLength(text, textSize);

    setPosition(top, bottom);
}

void Button::setFontSize(const uint32_t size) {
    if (textSize == size) {
        return;
    }
    textSize = size;
    length   = font->getLength(text, textSize);
    setPosition(top, bottom);
}

void Button::setPosition(const glm::vec2& topLeft,
                         const glm::vec2& bottomRight) {
    top.x = topLeft.x;
    top.y = topLeft.y;

    bottom.x = bottomRight.x;
    bottom.y = bottomRight.y;

    const auto width  = std::abs(topLeft.x - bottomRight.x);
    const auto height = std::abs(topLeft.y - bottomRight.y);

    if (alignType == ButtonAlignType::CenterAligned) {
        textPosition = { top.x + ((width - static_cast<float>(length)) / 2.F),
                         top.y +
                             ((height - static_cast<float>(textSize)) / 2.F) };
    } else {
        textPosition = { top.x + marginLeft,
                         top.y +
                             ((height - static_cast<float>(textSize)) / 2.F) };
    }
}

std::unique_ptr<Button> makeMenuButton(
    std::string_view message, const uint32_t fontSize,
    std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> font,
    const glm::vec4& buttonColor, const glm::vec3& textColor) {
    return std::make_unique<Button>(
        ButtonCreateInfo{ .message      = std::string(message),
                          .fontSize     = fontSize,
                          .font         = std::move(font),
                          .buttonColor  = buttonColor,
                          .textColor    = textColor,
                          .marginLeft   = 26,
                          .cornerRadius = 12.F,
                          .alignType    = ButtonAlignType::LeftAligned });
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

void updateButtonHover(Button* button, const glm::vec2& pos) {
    if (!button->hasHover() && button->isInside(pos)) {
        button->setHover(true);
    } else if (button->hasHover() && !button->isInside(pos)) {
        button->setHover(false);
    }
}
}  // namespace game::ui
