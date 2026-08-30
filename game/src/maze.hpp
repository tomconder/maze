#pragma once

#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "layer/exitlayer.hpp"
#include "layer/introlayer.hpp"
#include "layer/loadinglayer.hpp"
#include "layer/mazelayer.hpp"
#include "layer/splashscreenlayer.hpp"
#include "platform/glfw/core/application.hpp"

#ifdef ENABLE_IMGUI
#include "layer/imgui/imguilayer.hpp"
#else
#include "layer/imgui/noopimguilayer.hpp"
#endif

#include "layer/optionlayer.hpp"

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

    std::shared_ptr<layer::ExitLayer> getExitLayer() const {
        return exitLayer;
    }

    std::shared_ptr<layer::imgui::ImGuiLayer> getImGuiLayer() const {
        return imguiLayer;
    }

    std::shared_ptr<layer::IntroLayer> getIntroLayer() const {
        return introLayer;
    }

    std::shared_ptr<layer::LoadingLayer> getLoadingLayer() const {
        return loadingLayer;
    }

    std::shared_ptr<layer::MazeLayer> getMazeLayer() const {
        return mazeLayer;
    }

    std::shared_ptr<layer::OptionLayer> getOptionLayer() const {
        return optionLayer;
    }

    thread::AntiAliasing getAntiAliasing() const {
        return mazeLayer->getAntiAliasing();
    }

    void setAntiAliasing(thread::AntiAliasing val) {
        mazeLayer->setAntiAliasing(val);
    }

    bool isBloomEnabled() const {
        return mazeLayer->isBloomEnabled();
    }

    void setBloomEnabled(bool val) {
        mazeLayer->setBloomEnabled(val);
    }

    float getBloomThreshold() const {
        return mazeLayer->getBloomThreshold();
    }

    void setBloomThreshold(float val) {
        mazeLayer->setBloomThreshold(val);
    }

    float getBloomIntensity() const {
        return mazeLayer->getBloomIntensity();
    }

    void setBloomIntensity(float val) {
        mazeLayer->setBloomIntensity(val);
    }

    void exit() {
        isRunning = false;
    }

private:
    bool isRunning = true;

    std::shared_ptr<layer::ExitLayer> exitLayer =
        std::make_shared<layer::ExitLayer>();
    std::shared_ptr<layer::imgui::ImGuiLayer> imguiLayer =
        std::make_shared<layer::imgui::ImGuiLayer>();
    std::shared_ptr<layer::IntroLayer> introLayer =
        std::make_shared<layer::IntroLayer>();
    std::shared_ptr<layer::LoadingLayer> loadingLayer =
        std::make_shared<layer::LoadingLayer>();
    std::shared_ptr<layer::MazeLayer> mazeLayer =
        std::make_shared<layer::MazeLayer>();
    std::shared_ptr<layer::OptionLayer> optionLayer =
        std::make_shared<layer::OptionLayer>();
    std::shared_ptr<layer::SplashScreenLayer> splashScreenLayer =
        std::make_shared<layer::SplashScreenLayer>();

    bool onWindowClose(const sponge::event::WindowCloseEvent& event);
};
}  // namespace game
