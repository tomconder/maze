#include "maze.h"

#ifdef EMSCRIPTEN
#include <new>
#endif

#include "sponge.h"

constexpr std::array keyCodes = {
    sponge::KeyCode::SpongeKey_W,    sponge::KeyCode::SpongeKey_A,
    sponge::KeyCode::SpongeKey_S,    sponge::KeyCode::SpongeKey_D,
    sponge::KeyCode::SpongeKey_Up,   sponge::KeyCode::SpongeKey_Left,
    sponge::KeyCode::SpongeKey_Down, sponge::KeyCode::SpongeKey_Right
};

bool Maze::onUserCreate() {
    pushOverlay(hudLayer);
    pushLayer(mazeLayer);
    pushOverlay(exitLayer);

    exitLayer->setActive(false);

    return true;
}

bool Maze::onUserUpdate(const uint32_t elapsedTime) {
    if (!isRunning) {
        return false;
    }

    if (elapsedTime > 0) {
        for (const auto& keycode : keyCodes) {
            if (sponge::Input::isKeyPressed(keycode)) {
                auto event = sponge::KeyPressedEvent{ keycode, elapsedTime };
                onEvent(event);
            }
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
    dispatcher.dispatch<sponge::MouseButtonPressedEvent>(
        BIND_EVENT_FN(onMouseButtonPressed));
    dispatcher.dispatch<sponge::MouseButtonReleasedEvent>(
        BIND_EVENT_FN(onMouseButtonReleased));
    dispatcher.dispatch<sponge::WindowCloseEvent>(BIND_EVENT_FN(onWindowClose));

    sponge::SDLEngine::onEvent(event);
}

bool Maze::onKeyPressed(const sponge::KeyPressedEvent& event) {
    if (event.getKeyCode() == sponge::KeyCode::SpongeKey_Escape ||
        event.getKeyCode() == sponge::KeyCode::SpongeKey_Q) {
        if (exitLayer->isActive()) {
            if (!isMouseVisible) {
                setMouseVisible(false);
            }
            exitLayer->setActive(false);
        } else {
            exitLayer->setWidthAndHeight(getWidth(), getHeight());
            exitLayer->setActive(true);
            if (!isMouseVisible) {
                setMouseVisible(true);
            }
        }

        return true;
    }

    if (event.getKeyCode() == sponge::KeyCode::SpongeKey_F) {
        toggleFullscreen();
        return true;
    }

    return false;
}

bool Maze::onMouseButtonPressed(const sponge::MouseButtonPressedEvent& event) {
    if (!exitLayer->isActive()) {
        if (isMouseVisible && event.getMouseButton() == 0) {
            setMouseVisible(false);
            isMouseVisible = false;
        }
        return true;
    }
    return false;
}

bool Maze::onMouseButtonReleased(
    const sponge::MouseButtonReleasedEvent& event) {
    if (!exitLayer->isActive()) {
        if (!isMouseVisible && event.getMouseButton() == 0) {
            setMouseVisible(true);
            isMouseVisible = true;
        }
        return true;
    }
    return false;
}

bool Maze::onWindowClose(const sponge::WindowCloseEvent& event) {
    UNUSED(event);
    isRunning = false;
    return true;
}
