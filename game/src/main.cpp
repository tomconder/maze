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

extern "C" int main(int argc, char *args[]) {
    Sponge::Log::init();

    SPONGE_INFO("Starting maze");

#ifdef EMSCRIPTEN
    maze = std::make_unique<Maze>(800, 600);
#else
    maze = std::make_unique<Maze>(1600, 900);
#endif

    if (maze->construct() == 0) {
        return 1;
    }

    if (maze->start() == 0) {
        return 1;
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

    SPONGE_INFO("Shutting down");

    Sponge::Log::shutdown();

    return 0;
}
