#pragma once

#include "sponge.hpp"

namespace game::ui {

class Button {
   public:
    Button(const glm::vec2& topLeft, const glm::vec2& bottomRight,
           std::string_view message, uint32_t fontSize,
           std::string_view fontName, const glm::vec4& buttonColor,
           const glm::vec3& textColor);

    bool onUpdate(double elapsedTime) const;

    bool isInside(const glm::vec2& position) const;

    void setButtonColor(const glm::vec4& color);

    void setPosition(const glm::vec2& topLeft, const glm::vec2& bottomRight);

    void setHover(const bool value) {
        hover = value;
    }

    bool hasHover() const {
        return hover;
    }

   private:
    glm::vec2 top;
    glm::vec2 bottom;
    std::string text;
    uint32_t textSize;
    std::string textFontName;
    glm::vec4 buttonColor;
    glm::vec3 textColor;

    std::shared_ptr<sponge::platform::opengl::Font> font;
    std::unique_ptr<sponge::platform::opengl::Quad> quad;

    glm::vec2 textPosition;
    bool hover = false;
};

}  // namespace game::ui
