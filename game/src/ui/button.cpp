#include "ui/button.hpp"

#include <memory>
#include <string>
#include <utility>

namespace game::ui {
using sponge::platform::opengl::renderer::ResourceManager;

Button::Button(const glm::vec2& topLeft, const glm::vec2& bottomRight,
               std::string message, const uint32_t fontSize,
               std::string fontName, const glm::vec4& buttonColor,
               const glm::vec3& textColor) :
    top(topLeft),
    bottom(bottomRight),
    text(std::move(message)),
    textSize(fontSize),
    textFontName(std::move(fontName)),
    color(buttonColor),
    textColor(textColor),
    textPosition({ topLeft.x, topLeft.y }) {
    font = ResourceManager::getFont(textFontName);
    length = font->getLength(text, textSize);

    quad = std::make_unique<sponge::platform::opengl::scene::Quad>();
}

bool Button::onUpdate(const double elapsedTime) const {
    UNUSED(elapsedTime);

    quad->render(top, bottom, color);
    font->render(text, textPosition, textSize, textColor);

    return true;
}

bool Button::isInside(const glm::vec2& position) const {
    return top.x <= position.x && top.y <= position.y &&
           bottom.x >= position.x && bottom.y >= position.y;
}

void Button::setButtonColor(const glm::vec4& val) {
    color = glm::vec4(val);
}

void Button::setPosition(const glm::vec2& topLeft,
                         const glm::vec2& bottomRight) {
    top.x = topLeft.x;
    top.y = topLeft.y;

    bottom.x = bottomRight.x;
    bottom.y = bottomRight.y;

    const auto width = std::abs(topLeft.x - bottomRight.x);
    const auto height = std::abs(topLeft.y - bottomRight.y);

    textPosition = { top.x + ((width - static_cast<float>(length)) / 2.F),
                     top.y + ((height - static_cast<float>(textSize)) / 2.F) };
}
}  // namespace game::ui
