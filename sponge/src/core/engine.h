#pragma once

#include "core/base.h"
#include "core/input.h"
#include "event/event.h"
#include "renderer/opengl/openglcontext.h"
#include "renderer/opengl/openglinfo.h"
#include "renderer/opengl/openglrendererapi.h"
#include <string>

namespace sponge {

class Engine {
   public:
    virtual ~Engine() = default;

    virtual bool start() = 0;
    virtual bool iterateLoop() = 0;

    virtual bool onUserCreate() = 0;
    virtual bool onUserUpdate(const uint32_t elapsedTime) = 0;
    virtual bool onUserDestroy() = 0;

    virtual void onEvent(Event& event) = 0;
};

}  // namespace sponge
