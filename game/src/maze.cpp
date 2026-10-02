#include "maze.hpp"

#include "core/base.hpp"
#include "core/settings.hpp"
#include "entrypoint.hpp"
#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "version.hpp"

#include <charconv>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace game {
using sponge::platform::glfw::core::ApplicationSpecification;

Maze::Maze(ApplicationSpecification specification, const uint32_t captureFrames,
           const bool nanStats) :
    Application(std::move(specification)),
    captureFrames(captureFrames),
    nanStats(nanStats) {
    // Base class handles singleton pattern
}

bool Maze::onUserCreate() {
    pushOverlay(imguiLayer);
    imguiLayer->setActive(false);

    pushOverlay(splashScreenLayer);
    pushOverlay(exitLayer);
    pushOverlay(optionLayer);
    pushOverlay(keyMapLayer);
    pushOverlay(audioLayer);

    pushLayer(mazeLayer);
    pushLayer(loadingLayer);
    pushLayer(introLayer);

    exitLayer->setActive(false);
    introLayer->setActive(false);
    loadingLayer->setActive(false);
    mazeLayer->setActive(false);
    keyMapLayer->setActive(false);
    optionLayer->setActive(false);
    audioLayer->setActive(false);

    // A layer starts active, so the splash has to be turned off explicitly. Its
    // timeout is also what starts the intro.
    splashScreenLayer->setActive(captureFrames == 0);
    if (captureFrames > 0) {
        mazeLayer->enableCapture(captureFrames, nanStats);
    }

    const auto savedAa = sponge::core::Settings::getUInt32(
        "video.aa", static_cast<uint32_t>(thread::AntiAliasing::Taa));
    setAntiAliasing(savedAa <
                            static_cast<uint32_t>(thread::AntiAliasing::Count) ?
                        static_cast<thread::AntiAliasing>(savedAa) :
                        thread::AntiAliasing::Taa);
    setBloomEnabled(
        sponge::core::Settings::getBool("video.bloomEnabled", true));

    // After the settings above, so the maze is configured when it loads.
    if (captureFrames > 0) {
        loadingLayer->setActive(true);
    }

    return true;
}

bool Maze::onUserUpdate(const double elapsedTime) {
    if (!isRunning) {
        return false;
    }

    return Application::onUserUpdate(elapsedTime);
}

bool Maze::onUserDestroy() {
    return true;
}

void Maze::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);

    dispatcher.dispatch<sponge::event::WindowCloseEvent>(
        [this](const sponge::event::WindowCloseEvent& ev) {
            return this->onWindowClose(ev);
        });

    Application::onEvent(event);
}

bool Maze::onWindowClose(const sponge::event::WindowCloseEvent& event) {
    UNUSED(event);
    isRunning = false;
    return true;
}
}  // namespace game

std::unique_ptr<sponge::platform::glfw::core::Application>
    sponge::platform::glfw::core::createApplication(const int argc,
                                                    char**    argv) {
    using sponge::core::Settings;

    uint32_t captureFrames = 0;
    bool     nanStats      = false;

#ifndef NDEBUG
    // Debug builds only; a release build ignores both flags.
    // --capture-frames N or --capture-frames=N: skip the menus, render N
    // frames, write the last one as float maps and exit. --dump-nan-stats:
    // also scan every frame (N defaults to 600). A missing or non-numeric N
    // leaves the normal start.
    constexpr std::string_view          captureFlag = "--capture-frames";
    const std::vector<std::string_view> args{ argv + 1, argv + argc };
    const auto parseCount = [&captureFrames](const std::string_view value) {
        std::from_chars(value.data(), value.data() + value.size(),
                        captureFrames);
    };
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == captureFlag && i + 1 < args.size()) {
            parseCount(args[i + 1]);
        } else if (args[i].starts_with(captureFlag) &&
                   args[i].size() > captureFlag.size() &&
                   args[i][captureFlag.size()] == '=') {
            parseCount(args[i].substr(captureFlag.size() + 1));
        } else if (args[i] == "--dump-nan-stats") {
            nanStats = true;
        }
    }
    if (nanStats && captureFrames == 0) {
        captureFrames = 600;
    }
#else
    UNUSED(argc);
    UNUSED(argv);
#endif
    const bool capturing = captureFrames > 0;

    // A capture run uses a fixed window and no vsync, so runs compare.
    const auto spec = ApplicationSpecification{
        .name       = game::project_name,
        .width      = capturing ? 1600 : Settings::getUInt32("video.width", 0),
        .height     = capturing ? 900 : Settings::getUInt32("video.height", 0),
        .fullscreen = !capturing && Settings::getBool("video.fullscreen", true),
        .vsync      = !capturing && Settings::getBool("video.vsync", true),
    };

    return std::make_unique<game::Maze>(spec, captureFrames, nanStats);
}
