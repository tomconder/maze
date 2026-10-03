#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>

#include <string>

namespace game::scene {
struct OrthoCameraCreateInfo {
    std::string name;
};

class OrthoCamera final {
public:
    explicit OrthoCamera(const OrthoCameraCreateInfo& createInfo);

    const glm::mat4& getProjection() const {
        return projection;
    }

    void setWidthAndHeight(uint32_t width, uint32_t height);

    uint32_t getWidth() const {
        return w;
    }

    uint32_t getHeight() const {
        return h;
    }

private:
    void updateProjection(uint32_t width, uint32_t height);

    glm::mat4 projection = glm::mat4(1.F);

    uint32_t w = 0;
    uint32_t h = 0;
};

// One camera shared by the menu layers, so a layer attached later starts with
// the size the earlier ones already received.
std::shared_ptr<OrthoCamera> menuOrthoCamera();

}  // namespace game::scene
