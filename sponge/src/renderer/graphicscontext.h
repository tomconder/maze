#pragma once

#include "glm/vec4.hpp"

namespace Sponge {

class GraphicsContext {
   public:
    virtual ~GraphicsContext() = default;

    virtual void flip() = 0;
};

}  // namespace Sponge
