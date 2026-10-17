#pragma once

#include "platform/opengl/scene/quad.hpp"

#include <glm/glm.hpp>
#include <yoga/Yoga.h>

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

    // The row is a label that takes the free width, then the box. The caller
    // sizes and draws the label: part 0 of the row.
    void attach(YGNodeRef row);

    // y is where the row starts; the rows scroll and the nodes do not.
    void onUpdate(float y, bool checked);

    bool isInside(float mouseX, float mouseY, float y) const;

    void setSize(float newSize);

private:
    YGNodeRef rowNode = nullptr;
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
