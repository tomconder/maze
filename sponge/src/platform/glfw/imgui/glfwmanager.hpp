#pragma once

#include "platform/glfw/core/window.hpp"
#include "platform/glfw/imgui/imguimanager.hpp"

namespace sponge::platform::glfw::imgui {

class GLFWManager final : public ImGuiManager {
public:
    void onAttach(GLFWwindow* window) override;
    void onDetach() override;
    bool isEventHandled() override;
    void begin() override;
    void end() override;
};
}  // namespace sponge::platform::glfw::imgui
