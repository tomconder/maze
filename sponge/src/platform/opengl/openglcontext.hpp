#pragma once

#include "renderer/graphicscontext.hpp"
#include <string>

struct SDL_Window;

namespace sponge::platform::opengl {

class OpenGLContext : public renderer::GraphicsContext {
   public:
    explicit OpenGLContext(SDL_Window* window);
    ~OpenGLContext() override;

    void flip(void* window) override;

    void toggleFullscreen(void* window);

   private:
    int width = 0;
    int height = 0;
    std::string glName;
    bool isFullScreen = false;
};

}  // namespace sponge::platform::opengl
