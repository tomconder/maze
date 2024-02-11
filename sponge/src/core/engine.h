#pragma once

#include "event/event.h"

namespace sponge {

class Engine {
   public:
    virtual ~Engine() = default;

    virtual bool start() = 0;
    virtual bool iterateLoop() = 0;
    virtual void shutdown() = 0;

    virtual bool onUserCreate() = 0;
    virtual bool onUserUpdate(double elapsedTime) = 0;
    virtual bool onUserDestroy() = 0;

    virtual void onEvent(event::Event& event) = 0;
};

}  // namespace sponge
