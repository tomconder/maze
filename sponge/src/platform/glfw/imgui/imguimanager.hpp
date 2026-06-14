#pragma once

#include "platform/glfw/core/window.hpp"

namespace sponge::platform::glfw::imgui {
class ImGuiManager {
public:
    virtual ~ImGuiManager() = default;

    virtual void onAttach(GLFWwindow* window) = 0;
    virtual void onDetach()                   = 0;
    virtual bool isEventHandled()             = 0;
    virtual void begin()                      = 0;
    virtual void end()                        = 0;
};
}  // namespace sponge::platform::glfw::imgui
