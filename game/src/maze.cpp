#include "maze.h"

#ifdef EMSCRIPTEN
#include <new>
#endif

#include "sponge.h"

bool Maze::onUserCreate() {
    pushOverlay(hudLayer);
    pushLayer(mazeLayer);

    return true;
}

bool Maze::onUserUpdate(Uint32 elapsedTime) {
    if (!isRunning) {
        return false;
    }

    if (elapsedTime > 0) {
        sponge::KeyPressedEvent event =
            sponge::KeyPressedEvent{ sponge::KeyCode::SpongeKey_None };

        if (sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_W)) {
            event = sponge::KeyPressedEvent{ sponge::KeyCode::SpongeKey_W };
        } else if (sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_Up)) {
            event = sponge::KeyPressedEvent{ sponge::KeyCode::SpongeKey_Up };
        } else if (sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_S)) {
            event = sponge::KeyPressedEvent{ sponge::KeyCode::SpongeKey_S };
        } else if (sponge::Input::isKeyPressed(
                       sponge::KeyCode::SpongeKey_Down)) {
            event = sponge::KeyPressedEvent{ sponge::KeyCode::SpongeKey_Down };
        } else if (sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_A)) {
            event = sponge::KeyPressedEvent{ sponge::KeyCode::SpongeKey_A };
        } else if (sponge::Input::isKeyPressed(
                       sponge::KeyCode::SpongeKey_Left)) {
            event = sponge::KeyPressedEvent{ sponge::KeyCode::SpongeKey_Left };
        } else if (sponge::Input::isKeyPressed(sponge::KeyCode::SpongeKey_D)) {
            event = sponge::KeyPressedEvent{ sponge::KeyCode::SpongeKey_D };
        } else if (sponge::Input::isKeyPressed(
                       sponge::KeyCode::SpongeKey_Right)) {
            event = sponge::KeyPressedEvent{ sponge::KeyCode::SpongeKey_Right };
        }

        if (event.getKeyCode() != sponge::KeyCode::SpongeKey_None) {
            onEvent(event);
        }
    }

    return sponge::SDLEngine::onUserUpdate(elapsedTime);
}

bool Maze::onUserDestroy() {
    return true;
}

void Maze::onEvent(sponge::Event& event) {
    sponge::EventDispatcher dispatcher(event);
    dispatcher.dispatch<sponge::KeyPressedEvent>(BIND_EVENT_FN(onKeyPressed));
    dispatcher.dispatch<sponge::WindowCloseEvent>(BIND_EVENT_FN(onWindowClose));

    sponge::SDLEngine::onEvent(event);
}

bool Maze::onKeyPressed(const sponge::KeyPressedEvent& event) {
    if (event.getKeyCode() == sponge::KeyCode::SpongeKey_Escape ||
        event.getKeyCode() == sponge::KeyCode::SpongeKey_Q) {
        // isRunning = false;
        if (hasExit) {
            popLayer(exitLayer);
            hasExit = false;
        } else {
            pushLayer(exitLayer);
            exitLayer->setWidthAndHeight(getWidth(), getHeight());
            hasExit = true;
        }

        return true;
    }

    if (event.getKeyCode() == sponge::KeyCode::SpongeKey_F) {
        toggleFullscreen();
        return true;
    }

    return false;
}

bool Maze::onWindowClose(const sponge::WindowCloseEvent& event) {
    UNUSED(event);
    isRunning = false;
    return true;
}
