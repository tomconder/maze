#pragma once

#include <string>

#include "base.h"
#include "input.h"
#include "renderer/opengl/openglcontext.h"
#include "renderer/opengl/openglrendererapi.h"

class Engine {
   public:
    virtual ~Engine() = default;

    virtual int construct() const = 0;

    virtual int start() = 0;

    virtual bool iterateLoop() = 0;

    virtual bool onUserCreate() = 0;

    virtual bool onUserUpdate(Uint32 elapsedTime) = 0;

    virtual bool onUserDestroy() = 0;

    virtual bool onUserResize(int width, int height) = 0;
};
