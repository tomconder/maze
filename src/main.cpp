#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif
#include <SDL.h>

#include "easylogging++.h"
#include "maze.h"

INITIALIZE_EASYLOGGINGPP

std::unique_ptr<Maze> maze;

// loop iteration is broken out like this for emscripten
bool iterateLoop()
{
    return maze->iterateLoop();
}

extern "C" int main(int argc, char *args[])
{
    START_EASYLOGGINGPP(argc, args);

    maze = std::make_unique<Maze>();

    if (maze->construct(globals::SCREEN_WIDTH, globals::SCREEN_HEIGHT) == globals::Retcode::FAIL) {
        return 1;
    }

    if (maze->start() == globals::Retcode::FAIL) {
        return 1;
    }

#ifdef EMSCRIPTEN
    emscripten_set_main_loop((em_callback_func)iterateLoop, 60, true);
#else
    bool quit = false;
    while (!quit) {
        quit = iterateLoop();
    }
#endif

    return 0;
}
