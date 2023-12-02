#pragma once

#include "camera.h"
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace sponge::graphics {

class OrthoCamera : public Camera {
   public:
    const glm::mat4& getProjection() const {
        return projection;
    }

    void setWidthAndHeight(uint32_t width, uint32_t height);

    uint32_t getWidth() const {
        return w;
    }

    uint32_t getHeight() const {
        return h;
    }

   private:
    void updateProjection(uint32_t width, uint32_t height);

    glm::mat4 projection = glm::mat4(1.F);

    uint32_t w;
    uint32_t h;
};

}  // namespace sponge::graphics
