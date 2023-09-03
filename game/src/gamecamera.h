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

    float fov = 45.F;
    static constexpr float zNear = 1.F;
    static constexpr float zFar = 18000.F;

    float pitch = 0.F;
    float yaw = -90.F;

#ifdef __WINDOWS__
    static constexpr float keyboardSpeed = .1F;
#else
    static constexpr float keyboardSpeed = 1.F;
#endif

#ifdef __EMSCRIPTEN__
    static constexpr float mouseSpeed = .5F;
#else
    static constexpr float mouseSpeed = .1F;
#endif

    float width = 0.F;
    float height = 0.F;

    glm::mat4 projection = glm::mat4(1.F);
    glm::mat4 view = glm::mat4(1.F);
    glm::mat4 model = glm::mat4(1.F);

    // mvp = model * view * projection
    glm::mat4 mvp = glm::mat4(1.F);

    glm::vec3 cameraPos = { 0.F, 0.F, 1.F };
    static constexpr glm::vec3 up = { 0.F, 1.F, 0.F };
    glm::vec3 cameraFront = { 0.F, 0.F, -1.F };
};
