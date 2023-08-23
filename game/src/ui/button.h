#pragma once

#include "sponge.h"

namespace ui {

class Button {
   public:
    Button(const glm::vec2& topLeft, const glm::vec2& bottomRight,
           std::string_view message, uint32_t textSize,
           const glm::vec4& buttonColor, const glm::vec3& textColor);

    bool onUpdate(uint32_t elapsedTime);
    void onEvent(sponge::Event& event);

    void updateTopAndBottom(const glm::vec2& topLeft,
                            const glm::vec2& bottomRight);

   private:
    std::shared_ptr<sponge::OpenGLFont> font;
    std::unique_ptr<sponge::OpenGLQuad> quad;
    std::string_view text;

    glm::vec2 top;
    glm::vec2 bottom;
    glm::vec4 buttonColor;
    glm::vec3 textColor;

    glm::vec2 textPosition;
    uint32_t textSize;
};

}  // namespace ui