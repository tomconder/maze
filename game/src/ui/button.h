#pragma once

#include "sponge.h"

namespace ui {

class Button {
   public:
    Button(const glm::vec2& topLeft, const glm::vec2& bottomRight,
           std::string_view message, uint32_t fontSize,
           const glm::vec4& buttonColor, const glm::vec3& textColor);

    bool onUpdate(uint32_t elapsedTime);
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
    std::string_view text;
    uint32_t textSize;
    glm::vec4 buttonColor;
    glm::vec3 textColor;

    std::shared_ptr<sponge::OpenGLFont> font;
    std::unique_ptr<sponge::OpenGLQuad> quad;

    glm::vec2 textPosition;
    bool hover = false;
};

}  // namespace ui
