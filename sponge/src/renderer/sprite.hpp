#pragma once

#include <glm/vec2.hpp>

namespace sponge::renderer {

class Sprite {
   public:
    virtual ~Sprite() = default;

    virtual void render(glm::vec2 position, glm::vec2 size) const = 0;
};

}  // namespace sponge::renderer
