#pragma once

#include "sponge.h"
#include <glm/mat4x4.hpp>

class GameCamera : public sponge::Camera {
   public:
    GameCamera();
    void setViewportSize(uint32_t viewportWidth, uint32_t viewportHeight);

    const glm::mat4& getMVP() const {
        return mvp;
    }

    void setPosition(const glm::vec3& position);
    glm::vec3 getPosition() const {
        return cameraPos;
    }

    void moveForward(unsigned int delta);
    void moveBackward(unsigned int delta);
    void strafeLeft(unsigned int delta);
    void strafeRight(unsigned int delta);

    void mouseMove(const glm::vec2& offset);
    void mouseScroll(const glm::vec2& offset);

   private:
    void updateProjection();
    void updateView();

    float fov = 45.f;
    float zNear = 1.f;
    float zFar = 18000.f;

    float pitch = 0.f;
    float yaw = -90.f;

#ifdef __WINDOWS__
    float keyboardSpeed = .1f;
#else
    float keyboardSpeed = 1.f;
#endif

#ifdef __EMSCRIPTEN__
    float mouseSpeed = .5f;
#else
    float mouseSpeed = .1f;
#endif

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
