#include "ui/button.hpp"

#include <memory>

namespace {
constexpr float MARGIN_LEFT_OFFSET = 20.F;
}

namespace game::ui {
using sponge::platform::opengl::renderer::AssetManager;

Button::Button(const ButtonCreateInfo& createInfo) :
    top(createInfo.topLeft),
    bottom(createInfo.bottomRight),
    text(createInfo.message),
    textSize(createInfo.fontSize),
    textFontName(createInfo.fontName),
    color(createInfo.buttonColor),
    textColor(createInfo.textColor),
    cornerRadius(createInfo.cornerRadius),
    alignType(createInfo.alignType),
    textPosition({ createInfo.topLeft.x, createInfo.topLeft.y }) {
    font   = AssetManager::getFont(textFontName);
    length = font->getLength(text, textSize);

    quad = std::make_unique<sponge::platform::opengl::scene::Quad>();
}

bool Button::onUpdate(const double elapsedTime) const {
    UNUSED(elapsedTime);

    quad->render(top, bottom, color, cornerRadius);
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

void Button::setTextColor(const glm::vec3& val) {
    textColor = glm::vec3(val);
}

void Button::setAlignType(const ButtonAlignType& val) {
    alignType = val;
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
        textPosition = { top.x + MARGIN_LEFT_OFFSET,
                         top.y +
                             ((height - static_cast<float>(textSize)) / 2.F) };
    }
}
}  // namespace game::ui
