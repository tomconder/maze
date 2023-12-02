#pragma once

#include "glm/vec4.hpp"

namespace sponge::graphics::renderer {

class GraphicsContext {
   public:
    virtual ~GraphicsContext() = default;

    virtual void flip(void* window) = 0;
};

}  // namespace sponge
