#include "maze.h"

#ifdef EMSCRIPTEN
#include <new>
#endif

#include "sponge.h"

bool Maze::onUserCreate() {
    pushOverlay(new HUDLayer());
    pushLayer(new MazeLayer());

    return true;
}

bool Maze::onUserUpdate(Uint32 elapsedTime) {
    if (!isRunning) {
        return false;
    }

    return Sponge::SDLEngine::onUserUpdate(elapsedTime);
}

bool Maze::onUserDestroy() {
    return true;
}

void Maze::onEvent(Sponge::Event& event) {
    Sponge::EventDispatcher dispatcher(event);
    dispatcher.dispatch<Sponge::KeyPressedEvent>(BIND_EVENT_FN(onKeyPressed));
    dispatcher.dispatch<Sponge::WindowCloseEvent>(BIND_EVENT_FN(onWindowClose));

    Sponge::SDLEngine::onEvent(event);
}

bool Maze::onKeyPressed(const Sponge::KeyPressedEvent& event) {
    if (event.getKeyCode() == Sponge::KeyCode::SpongeKey_Escape || event.getKeyCode() == Sponge::KeyCode::SpongeKey_Q) {
        isRunning = false;
        return true;
    }

    if (event.getKeyCode() == Sponge::KeyCode::SpongeKey_F) {
        toggleFullscreen();
        return true;
    }

    return false;
}

bool Maze::onWindowClose(const Sponge::WindowCloseEvent& event) {
    UNUSED(event);
    isRunning = false;
    return true;
}
