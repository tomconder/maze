#include "graphics/camera/orthocamera.h"
#include <glm/ext/matrix_clip_space.hpp>

namespace sponge::graphics::camera {

void OrthoCamera::setWidthAndHeight(uint32_t width, uint32_t height) {
    w = width;
    h = height;
    updateProjection(w, h);
}

void OrthoCamera::updateProjection(uint32_t width, uint32_t height) {
    projection = glm::ortho(0.F, static_cast<float>(width),
                            static_cast<float>(height), 0.F, -1.F, 1.F);
}

}  // namespace sponge::graphics
