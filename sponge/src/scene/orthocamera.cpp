#include "scene/orthocamera.h"
#include <glm/ext/matrix_clip_space.hpp>

namespace sponge {

void OrthoCamera::setWidthAndHeight(uint32_t width, uint32_t height) {
    w = static_cast<float>(width);
    h = static_cast<float>(height);
    updateProjection(w, h);
}

void OrthoCamera::updateProjection(float width, float height) {
    projection = glm::ortho(0.F, width, 0.F, height, -1.F, 1.F);
}

}  // namespace sponge
