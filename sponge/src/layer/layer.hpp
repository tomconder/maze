#pragma once

#include "event/event.hpp"

#include <string>

namespace sponge::layer {
class Layer {
public:
    explicit Layer(std::string name = "undefined");
    virtual ~Layer() = default;

    virtual void onAttach() {}

    virtual void onDetach() {}

    virtual void onEvent(event::Event& event) {
        UNUSED(event);
    }

    virtual void onImGuiRender() {}

    virtual bool onUpdate(const double elapsedTime) {
        UNUSED(elapsedTime);
        return true;
    }

    // GPU commands using state from onUpdate(). No-op for render-thread layers.
    virtual void onRender() {}

    // True if onUpdate() runs on the update thread (GL-free; onRender() does
    // GPU work).
    virtual bool runsOnUpdateThread() const {
        return false;
    }

    const std::string& getName() const {
        return debugName;
    }

    bool isActive() const {
        return active;
    }

    void setActive(const bool value) {
        active = value;
    }

protected:
    std::string debugName;

private:
    bool active = true;
};
}  // namespace sponge::layer
