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

    // TODO use layer stack

    hudLayer = std::make_unique<HUDLayer>();
    hudLayer->onAttach();
    hudLayer->onResize(w, h);

    mazeLayer = std::make_unique<MazeLayer>();
    mazeLayer->onAttach();
    mazeLayer->onResize(w, h);

    return true;
}

bool Maze::onUserUpdate(Uint32 elapsedTime) {
    if (!isRunning) {
        return false;
    }

    // TODO use layer stack

    mazeLayer->onUpdate(elapsedTime);
    hudLayer->onUpdate(elapsedTime);

    return true;
}

bool Maze::onKeyPressed(Sponge::KeyPressedEvent& event) {
    if (event.getKeyCode() == Sponge::KeyCode::Escape || event.getKeyCode() == Sponge::KeyCode::Q) {
        isRunning = false;
    }

    if (event.getKeyCode() == Sponge::KeyCode::F) {
        graphics->toggleFullscreen();
    }

    return true;
}

bool Maze::onWindowClose(Sponge::WindowCloseEvent& event) {
    UNUSED(event);
    isRunning = false;
    return true;
}

bool Maze::onUserDestroy() {
    return true;
}

bool Maze::onEvent(Sponge::Event& event) {
    Sponge::EventDispatcher dispatcher(event);
    dispatcher.dispatch<Sponge::WindowCloseEvent>(BIND_EVENT_FN(onWindowClose));

    // TODO: replace with LayerStack

    //    for (auto it = layerStack.end(); it != layerStack.begin(); )
    //    {
    //        (*--it)->onEvent(event);
    //        if (event.handled) {
    //            break;
    //        }
    //    }

    hudLayer->onEvent(event);
    if (event.handled) {
        return true;
    }

    mazeLayer->onEvent(event);
    if (event.handled) {
        return true;
    }

    dispatcher.dispatch<Sponge::KeyPressedEvent>(BIND_EVENT_FN(onKeyPressed));

    return true;
}
