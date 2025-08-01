#pragma once

#include <glm/glm.hpp>

namespace sponge::scene {

class Sprite {
   public:
    virtual ~Sprite() = default;

    virtual void render(const glm::vec2& position,
                        const glm::vec2& size) const = 0;
};

}  // namespace sponge::scene
