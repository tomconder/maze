#include "button.h"

constexpr std::string_view gothicFont = "league-gothic";
constexpr std::string_view quadShader = "quad";

namespace ui {

Button::Button(const glm::vec2& topLeft, const glm::vec2& bottomRight,
               std::string_view message, uint32_t fontSize,
               const glm::vec4& buttonColor, const glm::vec3& textColor)
    : top(topLeft),
      bottom(bottomRight),
      text(message),
      textSize(fontSize),
      buttonColor(buttonColor),
      textColor(textColor) {
    font = sponge::OpenGLResourceManager::getFont(gothicFont.data());
    quad = std::make_unique<sponge::OpenGLQuad>();
}

bool Button::onUpdate(uint32_t elapsedTime) {
    UNUSED(elapsedTime);

    quad->render(top, bottom, buttonColor);
    font->render(text, textPosition, textSize, textColor);

    return false;
}

bool Button::isInside(const glm::vec2& position) const {
    return top.x <= position.x && top.y <= position.y &&
           bottom.x >= position.x && bottom.y >= position.y;
}

void Button::setButtonColor(const glm::vec4& color) {
    buttonColor.r = color.r;
    buttonColor.g = color.g;
    buttonColor.b = color.b;
    buttonColor.a = color.a;
}

void Button::setPosition(const glm::vec2& topLeft,
                         const glm::vec2& bottomRight) {
    top.x = topLeft.x;
    top.y = topLeft.y;

    bottom.x = bottomRight.x;
    bottom.y = bottomRight.y;

    auto width = std::abs(topLeft.x - bottomRight.x);
    auto height = std::abs(topLeft.y - bottomRight.y);

    auto length = font->getLength(text, textSize);
    textPosition = { top.x + (width - static_cast<float>(length)) / 2.F,
                     top.y + (height - static_cast<float>(textSize)) / 2.F };
}

}  // namespace ui