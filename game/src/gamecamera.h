#pragma once

#include <glm/mat4x4.hpp>

#include "sponge.h"

class GameCamera : public Sponge::Camera {
   public:
    GameCamera() = default;
    GameCamera(float fov, float width, float height, float zNear, float zFar);

    ~GameCamera() override = default;

    void setViewportSize(int viewportWidth, int viewportHeight);

    const glm::mat4 &getMVP() const {
        return mvp;
    }

    void setPosition(const glm::vec3 &position);
    glm::vec3 getPosition() {
        return cameraPos;
    }

    void moveForward(unsigned int delta);
    void moveBackward(unsigned int delta);
    void strafeLeft(unsigned int delta);
    void strafeRight(unsigned int delta);

    void mouseMove(const glm::vec2 &offset);
    void mouseScroll(const glm::vec2 &offset);

   private:
    void updateProjection();
    void updateView();

    float fov = 45.f;
    const float zNear = 1.f;
    const float zFar = 1000.f;

    float pitch = 0.f;
    float yaw = -90.f;

    float cameraSpeed = 0.1f;

    float width = 0.f;
    float height = 0.f;

    glm::mat4 projection = glm::mat4(1.f);
    glm::mat4 view = glm::mat4(1.f);
    glm::mat4 model = glm::mat4(1.f);

    // mvp = model * view * projection
    glm::mat4 mvp = glm::mat4(1.f);

    glm::vec3 cameraPos = { 0.f, 0.f, 1.f };
    glm::vec3 up = { 0.f, 1.f, 0.f };
    glm::vec3 cameraFront = { 0.f, 0.f, -1.f };
};
