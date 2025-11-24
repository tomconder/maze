#pragma once

#include "sponge.hpp"

#include <memory>
#include <string>

namespace game::ui {
struct ButtonCreateInfo {
    glm::vec2   topLeft;
    glm::vec2   bottomRight;
    std::string message;
    uint32_t    fontSize;
    std::string fontName;
    glm::vec4   buttonColor;
    glm::vec3   textColor;
    float       cornerRadius = 0.F;
};

class Button {
public:
    explicit Button(const ButtonCreateInfo& createInfo);

    bool onUpdate(double elapsedTime) const;

    bool isInside(const glm::vec2& position) const;

    void setButtonColor(const glm::vec4& val);

    void setPosition(const glm::vec2& topLeft, const glm::vec2& bottomRight);

    void setHover(const bool value) {
        hover = value;
    }

    bool hasHover() const {
        return hover;
    }

private:
    glm::vec2   top;
    glm::vec2   bottom;
    std::string text;
    uint32_t    textSize;
    std::string textFontName;
    glm::vec4   color;
    glm::vec3   textColor;
    uint32_t    length;
    float       cornerRadius;

    glm::vec2 textPosition;
    bool      hover = false;

    std::shared_ptr<sponge::platform::opengl::scene::Font> font;
    std::unique_ptr<sponge::platform::opengl::scene::Quad> quad;
};
}  // namespace game::ui
