#include "gamecamera.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

void GameCamera::updateProjection() {
    projection = glm::perspectiveFov(glm::radians(fov), width, height, zNear, zFar);
    mvp = projection * view;
}

void GameCamera::updateView() {
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, up);
    mvp = projection * view;
}

void GameCamera::setViewportSize(uint32_t viewportWidth, uint32_t viewportHeight) {
    width = static_cast<float>(viewportWidth);
    height = static_cast<float>(viewportHeight);
    updateProjection();
}

void GameCamera::setPosition(const glm::vec3 &position) {
    cameraPos = position;
    updateView();
}

void GameCamera::moveBackward(unsigned int delta) {
    cameraPos -= static_cast<float>(delta) * cameraFront * cameraSpeed;
    updateView();
}

void GameCamera::moveForward(unsigned int delta) {
    cameraPos += static_cast<float>(delta) * cameraFront * cameraSpeed;
    updateView();
}

void GameCamera::strafeLeft(unsigned int delta) {
    cameraPos -= glm::normalize(glm::cross(cameraFront, up)) * static_cast<float>(delta) * cameraSpeed;
    updateView();
}

void GameCamera::strafeRight(unsigned int delta) {
    cameraPos += glm::normalize(glm::cross(cameraFront, up)) * static_cast<float>(delta) * cameraSpeed;
    updateView();
}

void GameCamera::mouseMove(const glm::vec2 &offset) {
    yaw += offset.x * cameraSpeed;
    pitch += offset.y * cameraSpeed;

    pitch = glm::clamp(pitch, -89.f, 89.f);

    glm::vec3 front;
    front.x = glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
    front.y = glm::sin(glm::radians(pitch));
    front.z = glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
    updateView();
}

void GameCamera::mouseScroll(const glm::vec2 &offset) {
    if (offset.y == 0) {
        return;
    }

    fov -= offset.y * 5;
    fov = glm::clamp(fov, 30.f, 120.0f);
    updateProjection();
}
