#pragma once

#include <string>

#include "glm/vec4.hpp"
#include "renderer/graphicscontext.h"
#include "renderer/opengl/gl.h"

struct SDL_Window;

namespace Sponge {

class OpenGLContext : public GraphicsContext {
   public:
    OpenGLContext(SDL_Window *window, std::string name);

    ~OpenGLContext() override;

    void flip() override;

    void logGlVersion() const;

    void toggleFullscreen();

    static void logGraphicsDriverInfo();

    static void logOpenGLContextInfo();

    static void logStaticOpenGLInfo();

   private:
    SDL_Window *window = nullptr;
    int width = 0;
    int height = 0;
    std::string glName;
    int syncInterval = 0;
    bool isFullScreen = false;

    void setVSync(int interval);
};

}  // namespace Sponge
