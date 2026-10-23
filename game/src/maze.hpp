#pragma once

#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "layer/audiolayer.hpp"
#include "layer/confirmlayer.hpp"
#include "layer/exitlayer.hpp"
#include "layer/introlayer.hpp"
#include "layer/keymaplayer.hpp"
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

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace game {
#ifndef NDEBUG
// Debug builds only; the command-line flags fill it in.
struct CaptureOptions {
    // Non-zero skips the splash and intro screens, loads the maze and renders
    // that many frames, then exits. See FrameCapture.
    uint32_t    frames   = 0;
    bool        nanStats = false;
    std::string off;
    float       walk     = 0.F;
    uint32_t    cascades = 0;
    // Non-empty skips the splash, shows that menu screen and writes one
    // capture. See MenuCaptureLayer.
    std::string menu;
    // Non-empty loads assets/scenes/<scene>.yaml instead of maze.yaml.
    std::string scene;
};
#endif

class Maze final : public sponge::platform::glfw::core::Application {
public:
#ifdef NDEBUG
    explicit Maze(
        sponge::platform::glfw::core::ApplicationSpecification specification);
#else
    Maze(sponge::platform::glfw::core::ApplicationSpecification specification,
         CaptureOptions                                         capture = {});
#endif

    bool onUserCreate() override;

    bool onUserDestroy() override;

    bool onUserUpdate(double elapsedTime) override;

    void onEvent(sponge::event::Event& event) override;

    static Maze& get() {
        return static_cast<Maze&>(Application::get());
    }

    std::shared_ptr<layer::AudioLayer> getAudioLayer() const {
        return audioLayer;
    }

    std::shared_ptr<layer::ConfirmLayer> getConfirmLayer() const {
        return confirmLayer;
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

    std::shared_ptr<layer::KeyMapLayer> getKeyMapLayer() const {
        return keyMapLayer;
    }

    // The options screen is one layer per tab, and only one of them is ever
    // active — ask this rather than any single layer.
    bool isOptionsOpen() const {
        return optionLayer->isActive() || keyMapLayer->isActive() ||
               audioLayer->isActive();
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

    void exit() {
        isRunning = false;
    }

private:
    // The render thread clears it when a capture run ends.
    std::atomic<bool> isRunning{ true };

#ifndef NDEBUG
    CaptureOptions capture;

    void setUpCapture();
    void startCaptureLoad();
#endif

    std::shared_ptr<layer::AudioLayer> audioLayer =
        std::make_shared<layer::AudioLayer>();
    std::shared_ptr<layer::ConfirmLayer> confirmLayer =
        std::make_shared<layer::ConfirmLayer>();
    std::shared_ptr<layer::ExitLayer> exitLayer =
        std::make_shared<layer::ExitLayer>();
    std::shared_ptr<layer::imgui::ImGuiLayer> imguiLayer =
        std::make_shared<layer::imgui::ImGuiLayer>();
    std::shared_ptr<layer::IntroLayer> introLayer =
        std::make_shared<layer::IntroLayer>();
    std::shared_ptr<layer::KeyMapLayer> keyMapLayer =
        std::make_shared<layer::KeyMapLayer>();
    std::shared_ptr<layer::LoadingLayer> loadingLayer =
        std::make_shared<layer::LoadingLayer>();
    std::shared_ptr<layer::MazeLayer>   mazeLayer;
    std::shared_ptr<layer::OptionLayer> optionLayer =
        std::make_shared<layer::OptionLayer>();
    std::shared_ptr<layer::SplashScreenLayer> splashScreenLayer =
        std::make_shared<layer::SplashScreenLayer>();

    bool onWindowClose(const sponge::event::WindowCloseEvent& event);
};
}  // namespace game
