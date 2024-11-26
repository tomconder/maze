#pragma once

#include "event/event.hpp"
#include "platform/glfw/core/window.hpp"

namespace sponge::platform::glfw::imgui {

template <class T>
struct ImGuiManager {
    void onAttach(GLFWwindow* window) {
        static_cast<T*>(this)->onAttachImpl(window);
    }

    void onDetach() {
        static_cast<T*>(this)->onDetachImpl();
    }

    bool isEventHandled() {
        return static_cast<T*>(this)->isEventHandledImpl();
    }

    void begin() {
        static_cast<T*>(this)->beginImpl();
    }

    void end() {
        static_cast<T*>(this)->endImpl();
    }
};

}  // namespace sponge::platform::glfw::imgui
