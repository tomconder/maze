#pragma once

#include "core/engine.h"
#include "sdlinput.h"

class SDLEngine : public Engine {
   public:
    int construct() const override;

    int start() override;

    bool iterateLoop() override;

    bool onUserCreate() override;

    bool onUserUpdate(uint32_t elapsedTime) override;

    bool onUserDestroy() override;

    bool onUserResize(int width, int height) override;

    void adjustAspectRatio(int eventW, int eventH);

    static void logSDLVersion();

    SDLInput input;
    uint32_t lastUpdateTime = 0;
    std::string appName = "undefined";
    std::unique_ptr<OpenGLContext> graphics;
    std::unique_ptr<OpenGLRendererAPI> renderer;

    int offsetx = 0;
    int offsety = 0;
    int w = 0;
    int h = 0;
};

#define UNUSED(x) (void)(x)
