#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include "maze.h"
#include "sponge.h"

std::unique_ptr<Maze> maze;

// loop iteration is broken out like this for emscripten
bool iterateLoop() {
    return maze->iterateLoop();
}

bool startup(State &state, uint16_t startWidth, uint16_t startHeight) {
    Sponge::Log::init();

    SPONGE_INFO("Starting maze");

    state.name = "Maze";
    state.startWidth = startWidth;
    state.startHeight = startHeight;

    maze = std::make_unique<Maze>(state);

    if (maze->construct() == 0) {
        return false;
    }

    if (maze->start() == 0) {
        return false;
    }

    SPONGE_INFO("Iterating loop");

#ifdef EMSCRIPTEN
    emscripten_set_main_loop((em_callback_func)iterateLoop, 0, true);
#else
    bool quit = false;
    while (!quit) {
        quit = iterateLoop();
    }
#endif

    return true;
}

bool shutdown(const State &state) {
    UNUSED(state);

    SPONGE_INFO("Shutting down");

    Sponge::Log::shutdown();

    return true;
}

extern "C" int main(int argc, char *args[]) {
    UNUSED(argc);
    UNUSED(args);

    State state;

#ifdef EMSCRIPTEN
    startup(state, 800, 600);
#else
    startup(state, 1600, 900);
#endif

    shutdown(state);

    return 0;
}
