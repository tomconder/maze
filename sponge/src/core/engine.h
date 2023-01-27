#pragma once

#include <memory>
#include <string>

#include "input.h"
#include "platform/opengl/openglcontext.h"
#include "platform/opengl/openglrendererapi.h"

class Engine {
   public:
    virtual ~Engine() = default;

    int construct();

    int start();

    bool iterateLoop();

    static void logSDLVersion();

    virtual bool onUserCreate();

    virtual bool onUserUpdate(Uint32 elapsedTime);

    virtual bool onUserDestroy();

    virtual bool onUserResize(int width, int height);

    void adjustAspectRatio(int eventW, int eventH);

    Input input;
    Uint32 lastUpdateTime = 0;
    std::string appName = "undefined";
    std::unique_ptr<OpenGLContext> graphics;
    std::unique_ptr<OpenGLRendererAPI> renderer;

    int offsetx = 0;
    int offsety = 0;
    int w = 0;
    int h = 0;
};

#define UNUSED(x) (void)(x)
