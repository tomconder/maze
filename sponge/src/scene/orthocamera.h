#pragma once

#include "scene/camera.h"
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace sponge {

class OrthoCamera : public Camera {
   public:
    const glm::mat4& getProjection() const {
        return projection;
    }

    void setWidthAndHeight(uint32_t width, uint32_t height);

    float getWidth() const {
        return w;
    }

    float getHeight() const {
        return h;
    }

   private:
    void updateProjection(float width, float height);

    glm::mat4 projection = glm::mat4(1.F);

    float w;
    float h;
};

}  // namespace sponge
