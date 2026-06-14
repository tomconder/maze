#pragma once

#include "core/base.hpp"
#include "platform/glfw/imgui/imguimanager.hpp"

namespace sponge::platform::glfw::imgui {

class NoopManager final : public ImGuiManager {
public:
    void onAttach(GLFWwindow* window) override {
        UNUSED(window);
    }

    void onDetach() override { /* nothing */ }

    bool isEventHandled() override {
        return false;
    }

    void begin() override { /* nothing */ }

    void end() override { /* nothing */ }
};
}  // namespace sponge::platform::glfw::imgui
