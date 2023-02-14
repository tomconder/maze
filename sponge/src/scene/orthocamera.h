#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "camera.h"

class OrthoCamera : public Camera {
   public:
    OrthoCamera(float width, float height);
    ~OrthoCamera() override = default;

    const glm::mat4& getProjection() const {
        return projection;
    }

    void setWidthAndHeight(int width, int height);

   private:
    void updateProjection(float width, float height);

    glm::mat4 projection = glm::mat4(1.f);
};
