#include "maze.hpp"
#include "sponge.hpp"

namespace game {

Maze* Maze::instance = nullptr;

Maze::Maze() {
    assert(!instance && "Maze already exists!");
    instance = this;
}

bool Maze::onUserCreate() {
#if !NDEBUG
    pushOverlay(imguiLayer);
#endif
    pushOverlay(hudLayer);
    pushOverlay(exitLayer);

    pushLayer(gridLayer);
    pushLayer(mazeLayer);

    exitLayer->setActive(false);

    return true;
}

bool Maze::onUserUpdate(const double elapsedTime) {
    if (!isRunning) {
        return false;
    }

    return Engine::onUserUpdate(elapsedTime);
}

bool Maze::onUserDestroy() {
    return true;
}

void Maze::onEvent(sponge::event::Event& event) {
    sponge::event::EventDispatcher dispatcher(event);
    dispatcher.dispatch<sponge::event::KeyPressedEvent>(
        BIND_EVENT_FN(onKeyPressed));
    dispatcher.dispatch<sponge::event::WindowCloseEvent>(
        BIND_EVENT_FN(onWindowClose));

    Engine::onEvent(event);
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

    if (event.getKeyCode() == sponge::input::KeyCode::SpongeKey_GraveAccent) {
        if (imguiLayer->isActive()) {
            imguiLayer->setActive(false);
        } else {
            imguiLayer->setActive(true);
        }

        return true;
    }

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
