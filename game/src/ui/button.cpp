#include "button.h"

constexpr std::string_view gothicFont = "league-gothic";
constexpr std::string_view quadShader = "quad";

namespace ui {

Button::Button(const glm::vec2& topLeft, const glm::vec2& bottomRight,
               std::string_view message, uint32_t fontSize,
               const glm::vec4& buttonColor, const glm::vec3& textColor)
    : top(topLeft),
      bottom(bottomRight),
      buttonColor(buttonColor),
      textColor(textColor),
      textSize(fontSize) {
    text = std::string_view(message);
    font = sponge::OpenGLResourceManager::getFont(gothicFont.data());
    quad = std::make_unique<sponge::OpenGLQuad>();
}

bool Button::onUpdate(uint32_t elapsedTime) {
    UNUSED(elapsedTime);

    quad->render(top, bottom, buttonColor);
    font->render(text, textPosition, textSize, textColor);

    return false;
}

bool Button::isInside(const glm::vec2& position) {
    if (top.x <= position.x && top.y <= position.y && bottom.x >= position.x &&
        bottom.y >= position.y) {
        return true;
    }

    return false;
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

    float width = static_cast<float>(std::abs(topLeft.x - bottomRight.x));
    float height = static_cast<float>(std::abs(topLeft.y - bottomRight.y));

    auto length = font->getLength(text, textSize);
    textPosition = { top.x + (width - length) / 2.F,
                     top.y + (height - textSize) / 2.F };
}

}  // namespace ui
