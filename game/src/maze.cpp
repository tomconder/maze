#include "maze.h"
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
                auto event =
                    sponge::event::KeyPressedEvent{ keycode, elapsedTime };
                onEvent(event);
            }
        }
    }

    return sponge::SDLEngine::onUserUpdate(elapsedTime);
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
