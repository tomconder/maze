#pragma once

#include <glm/glm.hpp>
#include <string>

namespace game::scene {
struct GameCameraCreateInfo {
    std::string name;
};

class GameCamera final {
   public:
    GameCamera() = delete;
    explicit GameCamera(const GameCameraCreateInfo& createInfo);

    void setViewportSize(uint32_t viewportWidth, uint32_t viewportHeight);

    const glm::mat4& getMVP() const {
        return mvp;
    }

    void setPosition(const glm::vec3& position);

    glm::vec3 getPosition() const {
        return cameraPos;
    }

    float getFov() const {
        return fov;
    }

    void moveForward(double_t delta);

    void moveBackward(double_t delta);

    void strafeLeft(double_t delta);

    void strafeRight(double_t delta);

    void mouseMove(const glm::vec2& offset);

    void mouseScroll(const glm::vec2& offset);

   private:
    void updateProjection();

    void updateView();

    float fov = 60.F;

    static constexpr float zNear = 0.1F;
    static constexpr float zFar = 100.F;

    float pitch = -24.F;
    float yaw = 270.F;

    float width = 0.F;
    float height = 0.F;

    static constexpr float cameraSpeed = 100.F;

    glm::mat4 projection = glm::mat4(1.F);
    glm::mat4 view = glm::mat4(1.F);
    glm::mat4 model = glm::mat4(1.F);

    // mvp = model * view * projection
    glm::mat4 mvp = glm::mat4(1.F);

    glm::vec3 cameraPos = { 0.F, 0.F, 1.F };
    static constexpr glm::vec3 up = { 0.F, 1.F, 0.F };
    glm::vec3 cameraFront = { 0.F, 0.F, -1.F };
};
}  // namespace game::scene
