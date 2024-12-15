#pragma once

#include "imguimanager.hpp"
#include "platform/glfw/core/window.hpp"

namespace sponge::platform::glfw::imgui {

class GLFWManager : public ImGuiManager<GLFWManager> {
   public:
    static void onAttachImpl(GLFWwindow* window);

    static void onDetachImpl();

    static bool isEventHandledImpl();

    static void beginImpl();

    static void endImpl();
};
}  // namespace sponge::platform::glfw::imgui
