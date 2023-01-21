#pragma once

#include <string>

#include "gl.h"
#include "glm/vec4.hpp"
#include "renderer/graphicscontext.h"

struct SDL_Window;

class OpenGLContext : public GraphicsContext {
public:
    explicit OpenGLContext(SDL_Window *window, std::string name);

    ~OpenGLContext() override;

    void flip() override;

    void logGlVersion() const;

    void toggleFullscreen();

    static void logGraphicsDriverInfo();

    static void logOpenGLContextInfo();

    static void logStaticOpenGLInfo();

private:
    SDL_Window *window = nullptr;
    int width;
    int height;
    std::string glName;
    int syncInterval = 0;
    bool isFullScreen = false;

    void setVSync(int interval);
};
