#pragma once

#include "glm/vec4.hpp"
#include "renderer/graphicscontext.h"
#include "renderer/opengl/gl.h"
#include <string>

struct SDL_Window;

namespace Sponge {

class OpenGLContext : public GraphicsContext {
   public:
    OpenGLContext(SDL_Window* window, std::string name);
    ~OpenGLContext() override;

    void flip(void* window) override;

    void logGlVersion() const;

    void toggleFullscreen(void* window);

    static void logGraphicsDriverInfo();

    static void logOpenGLContextInfo();

    static void logStaticOpenGLInfo();

   private:
    int width = 0;
    int height = 0;
    std::string glName;
    bool isFullScreen = false;
};

}  // namespace Sponge
