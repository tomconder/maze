#pragma once

#include "event/event.hpp"

#include <memory>

namespace sponge::core {

class Application {
public:
    virtual ~Application() = default;

    virtual bool start()       = 0;
    virtual bool iterateLoop() = 0;
    virtual void shutdown()    = 0;

    virtual bool onUserCreate()                   = 0;
    virtual bool onUserUpdate(double elapsedTime) = 0;
    virtual bool onUserDestroy()                  = 0;

    virtual void onEvent(event::Event& event) = 0;

    virtual void run() = 0;
};

// implemented by the client
std::unique_ptr<Application> createApplication(int argc, char** argv);

}  // namespace sponge::core
