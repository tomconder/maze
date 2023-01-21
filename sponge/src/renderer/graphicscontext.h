#pragma once

#include "glm/vec4.hpp"

class GraphicsContext {
   public:
    virtual ~GraphicsContext() = default;

    virtual void flip() = 0;
};
