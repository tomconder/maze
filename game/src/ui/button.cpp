#include "button.hpp"

constexpr std::string_view quadShader = "quad";

namespace ui {

Button::Button(const glm::vec2& topLeft, const glm::vec2& bottomRight,
               std::string_view message, uint32_t fontSize,
               std::string_view fontName, const glm::vec4& buttonColor,
               const glm::vec3& textColor)
    : top(topLeft),
      bottom(bottomRight),
      text(message.data()),
      textSize(fontSize),
      textFontName(fontName.data()),
      buttonColor(buttonColor),
      textColor(textColor),
      textPosition({ topLeft.x, topLeft.y }) {
    font =
        sponge::platform::opengl::ResourceManager::getFont(textFontName);

    const auto shader =
        sponge::platform::opengl::ResourceManager::loadShader(
            "/shaders/quad.vert", "/shaders/quad.frag", quadShader.data());
    UNUSED(shader);

    quad = std::make_unique<sponge::platform::opengl::Quad>(
        quadShader.data());
}

bool Button::onUpdate(const double elapsedTime) const {
    UNUSED(elapsedTime);

    quad->render(top, bottom, buttonColor);
    font->render(text, textPosition, textSize, textColor);

    return true;
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

    const auto width = std::abs(topLeft.x - bottomRight.x);
    const auto height = std::abs(topLeft.y - bottomRight.y);

    const auto length = font->getLength(text, textSize);
    textPosition = { top.x + (width - static_cast<float>(length)) / 2.F,
                     top.y + (height - static_cast<float>(textSize)) / 2.F };
}

}  // namespace ui
