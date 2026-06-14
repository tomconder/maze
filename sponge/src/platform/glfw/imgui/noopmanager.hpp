#pragma once

#include "core/base.hpp"
#include "platform/glfw/core/window.hpp"

namespace sponge::platform::glfw::imgui {

class NoopManager final {
public:
    void onAttach(GLFWwindow* window) {
        UNUSED(window);
    }

    void onDetach() { /* nothing */ }

    bool isEventHandled() const {
        return false;
    }

    void begin() { /* nothing */ }

    void end() { /* nothing */ }
};
}  // namespace sponge::platform::glfw::imgui
