#pragma once

#include "sponge.hpp"

#include <memory>

namespace game::ui {

struct CheckboxCreateInfo {
    float     margin          = 0.F;
    float     size            = 32.F;
    float     cornerRadius    = 4.F;
    float     borderWidth     = 2.F;
    glm::vec4 checkedFill     = { 0.0F, 0.85F, 0.4F, 1.0F };
    glm::vec4 checkedBorder   = { 0.85F, 0.85F, 0.85F, 1.0F };
    glm::vec4 uncheckedFill   = { 0.08F, 0.08F, 0.08F, 1.0F };
    glm::vec4 uncheckedBorder = { 0.5F, 0.5F, 0.5F, 1.0F };
};

class Checkbox {
public:
    explicit Checkbox(const CheckboxCreateInfo& createInfo);

    void onUpdate(float x, float y, float w, float h, bool checked);

    bool isInside(float mouseX, float mouseY, float x, float y, float w,
                  float h) const;

private:
    float     margin;
    float     size;
    float     cornerRadius;
    float     borderWidth;
    glm::vec4 checkedFill;
    glm::vec4 checkedBorder;
    glm::vec4 uncheckedFill;
    glm::vec4 uncheckedBorder;

    std::unique_ptr<sponge::platform::opengl::scene::Quad> quad;
};

}  // namespace game::ui
