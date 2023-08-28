#pragma once

#include "sponge.h"

namespace ui {

class Button {
   public:
    Button(const glm::vec2& topLeft, const glm::vec2& bottomRight,
           std::string_view message, uint32_t fontSize,
           const glm::vec4& buttonColor, const glm::vec3& textColor);

    bool onUpdate(uint32_t elapsedTime);
    bool isInside(const glm::vec2& position);

    void setButtonColor(const glm::vec4& color);
    void setPosition(const glm::vec2& topLeft, const glm::vec2& bottomRight);

    void setHover(const bool value) { hover = value; }
    bool hasHover() { return hover; }

   private:
    std::shared_ptr<sponge::OpenGLFont> font;
    std::unique_ptr<sponge::OpenGLQuad> quad;
    std::string_view text;
    bool hover = false;

    glm::vec2 top;
    glm::vec2 bottom;
    glm::vec4 buttonColor;
    glm::vec3 textColor;

    glm::vec2 textPosition;
    uint32_t textSize;
};

}  // namespace ui
