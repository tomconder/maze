#pragma once

#include <glm/glm.hpp>
#include <optional>

namespace sponge::scene {

class Sprite {
public:
    virtual ~Sprite() = default;

    virtual void render(const glm::vec2& position, const glm::vec2& size,
                        std::optional<float> alpha = std::nullopt) const = 0;
};

}  // namespace sponge::scene
