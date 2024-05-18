#include "gamecamera.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace game::scene {

GameCamera::GameCamera() {
    updateView();

    const auto radYaw = glm::radians(yaw);
    const auto radPitch = glm::radians(pitch);
    cameraFront = { glm::cos(radYaw) * glm::cos(radPitch),  //
                    glm::sin(radPitch),                     //
                    glm::sin(radYaw) * glm::cos(radPitch) };
}

void GameCamera::updateProjection() {
    projection =
        glm::perspectiveFov(glm::radians(fov), width, height, zNear, zFar);
    mvp = projection * view;

    const float halfH = height / 2.F;
    const float halfW = width / 2.F;
    viewport = glm::mat4{ halfW, 0.F,   0.F, halfW,  //
                          0.F,   halfH, 0.F, halfH,  //
                          0.F,   0.F,   1.F, 0.F,    //
                          0.F,   0.F,   0.F, 1.F };
}

void GameCamera::updateView() {
    view = lookAt(cameraPos, cameraPos + cameraFront, up);
    mvp = projection * view;
}

void GameCamera::setViewportSize(uint32_t viewportWidth,
                                 uint32_t viewportHeight) {
    width = static_cast<float>(viewportWidth);
    height = static_cast<float>(viewportHeight);
    updateProjection();
}

void GameCamera::setPosition(const glm::vec3& position) {
    cameraPos = position;
    updateView();
}

void GameCamera::moveBackward(const double_t delta) {
    cameraPos -= static_cast<float>(delta * cameraSpeed) * cameraFront;
    updateView();
}

void GameCamera::moveForward(const double_t delta) {
    cameraPos += static_cast<float>(delta * cameraSpeed) * cameraFront;
    updateView();
}

void GameCamera::strafeLeft(const double_t delta) {
    cameraPos -= normalize(cross(cameraFront, up)) *
                 static_cast<float>(delta * cameraSpeed);
    updateView();
}

void GameCamera::strafeRight(const double_t delta) {
    cameraPos += normalize(cross(cameraFront, up)) *
                 static_cast<float>(delta * cameraSpeed);
    updateView();
}

void GameCamera::mouseMove(const glm::vec2& offset) {
    yaw += offset.x;
    pitch += offset.y;

    yaw = glm::mod(yaw, 360.F);
    pitch = glm::clamp(pitch, -89.F, 89.F);

    const auto radYaw = glm::radians(yaw);
    const auto radPitch = glm::radians(pitch);
    cameraFront =
        normalize(glm::vec3{ glm::cos(radYaw) * glm::cos(radPitch),  //
                             glm::sin(radPitch),                     //
                             glm::sin(radYaw) * glm::cos(radPitch) });

    updateView();
}

void GameCamera::mouseScroll(const glm::vec2& offset) {
    if (offset.y == 0) {
        return;
    }

    fov -= offset.y * 5;
    fov = glm::clamp(fov, 30.F, 120.0F);
    updateProjection();
}

}  // namespace game::scene
