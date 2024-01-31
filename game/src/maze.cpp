#include "maze.h"
#include "sponge.h"


bool Maze::onUserCreate() {
#if !NDEBUG
    pushOverlay(imguiLayer);
#endif
    pushOverlay(hudLayer);
    pushOverlay(exitLayer);

    pushLayer(mazeLayer);

    exitLayer->setActive(false);

    return true;
}

bool Maze::onUserUpdate(const uint32_t elapsedTime) {
    if (!isRunning) {
        return false;
    }

    return SDLEngine::onUserUpdate(elapsedTime);
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

    sponge::SDLEngine::onEvent(event);
}

bool Maze::onKeyPressed(const sponge::event::KeyPressedEvent& event) {
    if (event.getKeyCode() == sponge::KeyCode::SpongeKey_Escape) {
        if (exitLayer->isActive()) {
            exitLayer->setActive(false);
        } else {
            exitLayer->setWidthAndHeight(getWidth(), getHeight());
            exitLayer->setActive(true);
        }

        return true;
    }

    if (event.getKeyCode() == sponge::KeyCode::SpongeKey_GraveAccent) {
        if (imguiLayer->isActive()) {
            imguiLayer->setActive(false);
        } else {
            imguiLayer->setActive(true);
        }

        return true;
    }

    if (event.getKeyCode() == sponge::KeyCode::SpongeKey_F) {
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
