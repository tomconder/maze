#include "maze.hpp"

#include "core/base.hpp"
#include "core/settings.hpp"
#include "entrypoint.hpp"
#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "version.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace game {
using sponge::platform::glfw::core::ApplicationSpecification;

Maze::Maze(ApplicationSpecification specification) :
    Application(std::move(specification)) {
    // Base class handles singleton pattern
}

bool Maze::onUserCreate() {
    pushOverlay(imguiLayer);
    imguiLayer->setActive(false);

    pushOverlay(splashScreenLayer);
    pushOverlay(exitLayer);
    pushOverlay(optionLayer);
    pushOverlay(keyMapLayer);

    pushLayer(mazeLayer);
    pushLayer(loadingLayer);
    pushLayer(introLayer);

    exitLayer->setActive(false);
    introLayer->setActive(false);
    loadingLayer->setActive(false);
    mazeLayer->setActive(false);
    keyMapLayer->setActive(false);
    optionLayer->setActive(false);
    splashScreenLayer->setActive(true);

    const auto savedAa = sponge::core::Settings::getUInt32(
        "video.aa", static_cast<uint32_t>(thread::AntiAliasing::Taa));
    setAntiAliasing(savedAa <
                            static_cast<uint32_t>(thread::AntiAliasing::Count) ?
                        static_cast<thread::AntiAliasing>(savedAa) :
                        thread::AntiAliasing::Taa);
    setBloomEnabled(
        sponge::core::Settings::getBool("video.bloomEnabled", true));
    setBloomThreshold(std::stof(
        sponge::core::Settings::getString("video.bloomThreshold", "0.8")));
    setBloomIntensity(std::stof(
        sponge::core::Settings::getString("video.bloomIntensity", "0.08")));

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
    UNUSED(argc);
    UNUSED(argv);

    using sponge::core::Settings;

    const auto spec = ApplicationSpecification{
        .name       = game::project_name,
        .width      = Settings::getUInt32("video.width", 0),
        .height     = Settings::getUInt32("video.height", 0),
        .fullscreen = Settings::getBool("video.fullscreen", true),
        .vsync      = Settings::getBool("video.vsync", true),
    };

    return std::make_unique<game::Maze>(spec);
}
