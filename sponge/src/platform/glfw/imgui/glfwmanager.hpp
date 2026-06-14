#pragma once

#include "platform/glfw/core/window.hpp"

namespace sponge::platform::glfw::imgui {

class GLFWManager final {
public:
    void onAttach(GLFWwindow* window);
    void onDetach();
    bool isEventHandled() const;
    void begin();
    void end();
};
}  // namespace sponge::platform::glfw::imgui
