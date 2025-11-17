#include "maze.hpp"

#include "entrypoint.hpp"
#include "sponge.hpp"
#include "version.hpp"

namespace game {
using sponge::platform::glfw::core::ApplicationSpecification;

Maze* Maze::instance = nullptr;

Maze::Maze(const ApplicationSpecification& specification) :
    Application(specification) {
    assert(!instance && "Maze already exists!");
    instance = this;
}

bool Maze::onUserCreate() {
#if defined(ENABLE_IMGUI)
    pushOverlay(imguiLayer);
#endif
    pushOverlay(exitLayer);

    pushLayer(mazeLayer);

    exitLayer->setActive(false);

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
    dispatcher.dispatch<sponge::event::KeyPressedEvent>(
        [this](const sponge::event::KeyPressedEvent& ev) {
            return this->onKeyPressed(ev);
        });
    dispatcher.dispatch<sponge::event::WindowCloseEvent>(
        [this](const sponge::event::WindowCloseEvent& ev) {
            return this->onWindowClose(ev);
        });

    Application::onEvent(event);
}

bool Maze::onKeyPressed(const sponge::event::KeyPressedEvent& event) {
    if (event.getKeyCode() == sponge::input::KeyCode::SpongeKey_Escape) {
        if (exitLayer->isActive()) {
            exitLayer->setActive(false);
        } else {
            auto resizeEvent =
                sponge::event::WindowResizeEvent{ getWidth(), getHeight() };
            exitLayer->onEvent(resizeEvent);
            exitLayer->setActive(true);
        }

        return true;
    }

#if defined(ENABLE_IMGUI)
    if (event.getKeyCode() == sponge::input::KeyCode::SpongeKey_GraveAccent) {
        if (imguiLayer->isActive()) {
            imguiLayer->setActive(false);
        } else {
            imguiLayer->setActive(true);
        }

        return true;
    }
#endif

    if (event.getKeyCode() == sponge::input::KeyCode::SpongeKey_F) {
        toggleFullscreen();
        return true;
    }

    return false;
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
