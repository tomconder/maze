#pragma once

namespace sponge::renderer {

class GraphicsContext {
   public:
    virtual ~GraphicsContext() = default;

    virtual void flip(void* window) = 0;
};

}  // namespace sponge::renderer
