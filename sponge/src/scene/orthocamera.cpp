#include "scene/orthocamera.h"
#include <glm/ext/matrix_clip_space.hpp>

namespace Sponge {

void OrthoCamera::setWidthAndHeight(uint32_t width, uint32_t height) {
    w = static_cast<float>(width);
    h = static_cast<float>(height);
    updateProjection(w, h);
}

void OrthoCamera::updateProjection(float width, float height) {
    projection = glm::ortho(0.f, width, 0.f, height, -1.f, 1.f);
}

}  // namespace Sponge
