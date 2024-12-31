#pragma once

#include "core/base.hpp"
#include "imguimanager.hpp"

namespace sponge::platform::glfw::imgui {

class NoopManager : public ImGuiManager<NoopManager> {
   public:
    static void onAttachImpl(GLFWwindow* window) {
        UNUSED(window);
    }

    static void onDetachImpl() { /* nothing */ }

    static bool isEventHandledImpl() {
        return false;
    }

    static void beginImpl() { /* nothing */ }

    static void endImpl() { /* nothing */ }
};
}  // namespace sponge::platform::glfw::imgui
