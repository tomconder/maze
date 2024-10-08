#pragma once

#include "renderer/graphicscontext.hpp"

struct SDL_Window;

namespace sponge::platform::opengl::renderer {

class Context final : public sponge::renderer::GraphicsContext {
   public:
    explicit Context(SDL_Window* window);
    ~Context() override;

    void flip(void* window) override;
};

}  // namespace sponge::platform::opengl::renderer
