#pragma once

#include "event/event.hpp"

#include <string>
#include <utility>

namespace sponge::layer {
class Layer {
public:
    explicit Layer(std::string name = "undefined") :
        debugName(std::move(name)) {}
    virtual ~Layer() = default;

    virtual void onAttach() {}

    virtual void onDetach() {}

    virtual void onEvent([[maybe_unused]] event::Event& event) {}

    virtual void onImGuiRender() {}

    virtual bool onUpdate([[maybe_unused]] const double elapsedTime) {
        return true;
    }

    // GPU commands using state from onUpdate(). No-op for render-thread layers.
    virtual void onRender() {}

    // Called on the main thread while both worker threads are idle, before
    // update[N] and render[N] are kicked. Publish state produced by
    // update[N-1] here so render[N] sees a deterministic, completed frame.
    virtual void onFrameSync() {}

    // True if onUpdate() runs on the update thread (GL-free; onRender() does
    // GPU work).
    virtual bool runsOnUpdateThread() const {
        return false;
    }

    // True if this layer overwrites every pixel of the back buffer, so the
    // frame-start colour clear can be skipped. Without that, the clear colour
    // stays on screen while a long frame renders and shows as a flash.
    virtual bool coversScreen() const {
        return false;
    }

    // True for a menu screen. While one is active, Application runs the loop at
    // no more than 30 frames a second.
    virtual bool isMenu() const {
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
