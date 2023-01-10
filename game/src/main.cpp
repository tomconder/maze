#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include "core/log.h"
#include "maze.h"

std::unique_ptr<Maze> maze;

// loop iteration is broken out like this for emscripten
bool iterateLoop() {
    return maze->iterateLoop();
}

extern "C" int main(int argc, char *args[]) {
    Log::init();

    SPONGE_INFO("Starting maze");

    maze = std::make_unique<Maze>();

    if (maze->construct(globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT) == 0) {
        return 1;
    }

    if (maze->start() == 0) {
        return 1;
    }

    SPONGE_INFO("Iterating loop");

#ifdef EMSCRIPTEN
    emscripten_set_main_loop((em_callback_func)iterateLoop, 60, true);
#else
    bool quit = false;
    while (!quit) {
        quit = iterateLoop();
    }
#endif

    SPONGE_INFO("Shutting down");

    Log::shutdown();

    return 0;
}
