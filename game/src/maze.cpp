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

    pushLayer(mazeLayer);
    pushLayer(introLayer);

    introLayer->setActive(false);
    exitLayer->setActive(false);
    mazeLayer->setActive(false);

    return true;
}

bool Maze::onUserUpdate(const double elapsedTime) {
    if (splashScreenLayer && splashScreenLayer->isActive()) {
        if (splashScreenLayer->isFading() && !introLayer->isActive()) {
            introLayer->setActive(true);
        }

        if (splashScreenLayer->shouldDismiss()) {
            popOverlay(splashScreenLayer);
            introLayer->setActive(true);
            splashScreenLayer.reset();
        }
    }

    if (introLayer && introLayer->isActive()) {
        if (introLayer->shouldStartGame()) {
            introLayer->setActive(false);
            mazeLayer->setActive(true);
#ifdef ENABLE_IMGUI
            imguiLayer->setActive(true);
#endif
        }

        if (introLayer && introLayer->shouldQuit()) {
            isRunning = false;
        }
    }

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

    constexpr uint32_t width      = 1600;
    constexpr uint32_t height     = 900;
    constexpr bool     fullscreen = false;

    using platform::glfw::core::ApplicationSpecification;

    const auto spec = ApplicationSpecification{ .name   = game::project_name,
                                                .width  = width,
                                                .height = height,
                                                .fullscreen = fullscreen };

    return std::make_unique<game::Maze>(spec);
}
