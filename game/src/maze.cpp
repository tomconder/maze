#include "maze.h"

#ifdef EMSCRIPTEN
#include <new>
#endif

#include "sponge.h"
#include "version.h"

Maze::Maze(int screenWidth, int screenHeight) {
    std::string base = "Maze ";
    appName = base + MAZE_VERSION;
    SPONGE_INFO("Maze {}", MAZE_VERSION);
    w = screenWidth;
    h = screenHeight;
}

bool Maze::onUserCreate() {
    adjustAspectRatio(w, h);

    SPONGE_INFO("Setting camera for {}x{}", w, h);
    renderer->setViewport(offsetx, offsety, w, h);

    pushOverlay(new HUDLayer());
    pushLayer(new MazeLayer());

    auto resizeEvent = Sponge::WindowResizeEvent{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
    onEvent(resizeEvent);

    return true;
}

bool Maze::onUserUpdate(Uint32 elapsedTime) {
    if (!isRunning) {
        return false;
    }

    return Sponge::SDLEngine::onUserUpdate(elapsedTime);
}

bool Maze::onKeyPressed(Sponge::KeyPressedEvent& event) {
    if (event.getKeyCode() == Sponge::KeyCode::Escape || event.getKeyCode() == Sponge::KeyCode::Q) {
        isRunning = false;
        return true;
    }

    if (event.getKeyCode() == Sponge::KeyCode::F) {
        graphics->toggleFullscreen();
        return true;
    }

    return false;
}

bool Maze::onWindowClose(Sponge::WindowCloseEvent& event) {
    UNUSED(event);
    isRunning = false;
    return true;
}

bool Maze::onUserDestroy() {
    return true;
}

void Maze::onEvent(Sponge::Event& event) {
    Sponge::EventDispatcher dispatcher(event);
    dispatcher.dispatch<Sponge::WindowCloseEvent>(BIND_EVENT_FN(onWindowClose));
    dispatcher.dispatch<Sponge::KeyPressedEvent>(BIND_EVENT_FN(onKeyPressed));

    Sponge::SDLEngine::onEvent(event);
}
