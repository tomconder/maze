#pragma once

#include <glm/vec2.hpp>
#include <string>

namespace sponge::graphics::renderer {

class Sprite {
   public:
    virtual ~Sprite() = default;

    virtual void render(glm::vec2 position, glm::vec2 size) const = 0;
};

}  // namespace sponge::graphics::renderer