#include "orthocamera.h"

#include <glm/ext/matrix_clip_space.hpp>

OrthoCamera::OrthoCamera(float width, float height) {
    updateProjection(width, height);
}

void OrthoCamera::setWidthAndHeight(int width, int height) {
    updateProjection(static_cast<float>(width), static_cast<float>(height));
}

void OrthoCamera::updateProjection(float width, float height) {
    projection = glm::ortho(0.f, width, 0.f, height, -1.f, 1.f);
}
