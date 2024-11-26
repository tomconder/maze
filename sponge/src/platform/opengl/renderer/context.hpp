#pragma once

#include "renderer/graphicscontext.hpp"

struct GLFWwindow;

namespace sponge::platform::opengl::renderer {

class Context final : public sponge::renderer::GraphicsContext {
   public:
    explicit Context(GLFWwindow* window);

    void flip(void* window) override;
};

}  // namespace sponge::platform::opengl::renderer
