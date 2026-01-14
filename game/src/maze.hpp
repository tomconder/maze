#pragma once

#include "layer/exitlayer.hpp"
#include "layer/introlayer.hpp"
#include "layer/mazelayer.hpp"
#include "layer/splashscreenlayer.hpp"
#include "sponge.hpp"

#ifdef ENABLE_IMGUI
#include "layer/imgui/imguilayer.hpp"
#endif

#include <memory>

namespace game {
class Maze final : public sponge::platform::glfw::core::Application {
public:
    explicit Maze(
        sponge::platform::glfw::core::ApplicationSpecification specification);

    bool onUserCreate() override;

    bool onUserDestroy() override;

    bool onUserUpdate(double elapsedTime) override;

    void onEvent(sponge::event::Event& event) override;

    static Maze& get() {
        return static_cast<Maze&>(Application::get());
    }

    std::shared_ptr<layer::MazeLayer> getMazeLayer() const {
        return mazeLayer;
    }

#ifdef ENABLE_IMGUI
    std::shared_ptr<layer::imgui::ImGuiLayer> getImGuiLayer() const {
        return imguiLayer;
    }
#endif

    void exit() {
        isRunning = false;
    }

private:
    bool isRunning = true;

    std::shared_ptr<layer::SplashScreenLayer> splashScreenLayer =
        std::make_shared<layer::SplashScreenLayer>();
    std::shared_ptr<layer::IntroLayer> introLayer =
        std::make_shared<layer::IntroLayer>();
    std::shared_ptr<layer::ExitLayer> exitLayer =
        std::make_shared<layer::ExitLayer>();
#ifdef ENABLE_IMGUI
    std::shared_ptr<layer::imgui::ImGuiLayer> imguiLayer =
        std::make_shared<layer::imgui::ImGuiLayer>();
#endif
    std::shared_ptr<layer::MazeLayer> mazeLayer =
        std::make_shared<layer::MazeLayer>();

    bool onWindowClose(const sponge::event::WindowCloseEvent& event);
};
}  // namespace game
