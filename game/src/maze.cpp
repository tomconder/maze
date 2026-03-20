#include "maze.hpp"

#include "entrypoint.hpp"
#include "sponge.hpp"
#include "version.hpp"

namespace game {
using sponge::platform::glfw::core::ApplicationSpecification;

Maze::Maze(ApplicationSpecification specification) :
    Application(std::move(specification)) {
    // Base class handles singleton pattern
}

bool Maze::onUserCreate() {
#ifdef ENABLE_IMGUI
    pushOverlay(imguiLayer);
    imguiLayer->setActive(false);
#endif

    pushOverlay(splashScreenLayer);
    pushOverlay(exitLayer);
    pushOverlay(optionLayer);

    pushLayer(mazeLayer);
    pushLayer(introLayer);

    exitLayer->setActive(false);
    introLayer->setActive(true);
    mazeLayer->setActive(false);
    optionLayer->setActive(false);
    splashScreenLayer->setActive(true);

    setFxaaEnabled(sponge::core::Settings::getBool("video.fxaa", true));

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

std::unique_ptr<sponge::core::Application>
    sponge::core::createApplication(const int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    using core::Settings;
    using platform::glfw::core::ApplicationSpecification;

    const auto spec = ApplicationSpecification{
        .name       = game::project_name,
        .width      = Settings::getUInt32("video.width", 0),
        .height     = Settings::getUInt32("video.height", 0),
        .fullscreen = Settings::getBool("video.fullscreen", true),
        .vsync      = Settings::getBool("video.vsync", true)
    };

    return std::make_unique<game::Maze>(spec);
}
