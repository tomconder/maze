#include "maze.h"

#ifdef EMSCRIPTEN
#include <new>
#endif

#include "sponge.h"

#define IS_KEY_PRESSED(event, keycode)              \
    if (sponge::Input::isKeyPressed(keycode)) {     \
        event = sponge::KeyPressedEvent{ keycode }; \
    }

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

        IS_KEY_PRESSED(event, sponge::KeyCode::SpongeKey_W)
        IS_KEY_PRESSED(event, sponge::KeyCode::SpongeKey_A)
        IS_KEY_PRESSED(event, sponge::KeyCode::SpongeKey_S)
        IS_KEY_PRESSED(event, sponge::KeyCode::SpongeKey_D)

        IS_KEY_PRESSED(event, sponge::KeyCode::SpongeKey_Up)
        IS_KEY_PRESSED(event, sponge::KeyCode::SpongeKey_Left)
        IS_KEY_PRESSED(event, sponge::KeyCode::SpongeKey_Down)
        IS_KEY_PRESSED(event, sponge::KeyCode::SpongeKey_Right)

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
            auto resizeEvent =
                sponge::WindowResizeEvent{ getWidth(), getHeight() };
            onEvent(resizeEvent);

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
