#include "scene/gamecamera.hpp"

#include "logging/log.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>

#include <cstdint>

namespace game::scene {
GameCamera::GameCamera(
    [[maybe_unused]] const GameCameraCreateInfo& createInfo) {
    SPONGE_INFO("Creating game camera: {}", createInfo.name);

    setOrientation(0.F, 0.F);
}

void GameCamera::updateProjection() {
    projection =
        glm::perspectiveFov(glm::radians(fov), width, height, zNear, zFar);
    mvp = projection * view;
}

void GameCamera::updateView() {
    // The orientation's own axes are the camera basis (+X front, +Y up, +Z
    // right), so the view needs no lookAt. lookAt takes a cross product with
    // the world up axis, which loses precision as the front nears a pole.
    const glm::mat3 basis = glm::mat3_cast(orientation);
    const glm::vec3 right = basis[2];
    const glm::vec3 local = basis[1];
    view = glm::mat4(glm::vec4(right.x, local.x, -cameraFront.x, 0.F),
                     glm::vec4(right.y, local.y, -cameraFront.y, 0.F),
                     glm::vec4(right.z, local.z, -cameraFront.z, 0.F),
                     glm::vec4(-glm::dot(right, cameraPos),
                               -glm::dot(local, cameraPos),
                               glm::dot(cameraFront, cameraPos), 1.F));
    mvp  = projection * view;
}

void GameCamera::setViewportSize(const uint32_t viewportWidth,
                                 const uint32_t viewportHeight) {
    width  = static_cast<float>(viewportWidth);
    height = static_cast<float>(viewportHeight);
    updateProjection();
}

void GameCamera::setPosition(const glm::vec3& position) {
    cameraPos = position;
    updateView();
}

void GameCamera::setOrientation(const float yawDegrees,
                                const float pitchDegrees) {
    const auto pitch = glm::clamp(pitchDegrees, -maxPitch, maxPitch);

    orientation = glm::angleAxis(glm::radians(yawDegrees), -up) *
                  glm::angleAxis(glm::radians(pitch), pitchAxis);
    applyOrientation();
}

void GameCamera::moveBackward(const double_t delta) {
    cameraPos -= static_cast<float>(delta * cameraSpeed) * cameraFront;
    updateView();
}

void GameCamera::setFov(const float value) {
    fov = value;
    updateProjection();
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
    // Yaw about the world up axis, pitch about the camera's own right axis,
    // with the pitch held inside the pole limit.
    const auto currentPitch = glm::degrees(glm::atan(
        cameraFront.y, glm::length(glm::vec2(cameraFront.x, cameraFront.z))));
    const auto pitch =
        glm::clamp(currentPitch + offset.y, -maxPitch, maxPitch) - currentPitch;

    orientation = glm::angleAxis(glm::radians(offset.x), -up) * orientation *
                  glm::angleAxis(glm::radians(pitch), pitchAxis);
    applyOrientation();
}

void GameCamera::applyOrientation() {
    // Renormalize so repeated deltas do not drift off a unit quaternion.
    orientation = glm::normalize(orientation);
    cameraFront = orientation * identityFront;
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
