#include "button.h"

constexpr std::string_view gothicFont = "league-gothic";
constexpr std::string_view quadShader = "quad";

namespace ui {

Button::Button(const glm::vec2& topLeft, const glm::vec2& bottomRight,
               std::string_view message, uint32_t textSize,
               const glm::vec4& buttonColor, const glm::vec3& textColor)
    : top(topLeft),
      bottom(bottomRight),
      buttonColor(buttonColor),
      textColor(textColor),
      textSize(textSize) {
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

void Button::onEvent(sponge::Event& event) {
    UNUSED(event);
}

void Button::updateTopAndBottom(const glm::vec2& topLeft,
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
