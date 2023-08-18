#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include "maze.h"
#include "sponge.h"
#include "version.h"

std::unique_ptr<Maze> maze;

// loop iteration is broken out like this for emscripten
bool iterateLoop() {
    return maze->iterateLoop();
}

bool startup() {
    auto logfile = sponge::File::getLogDir() + SPONGE_LOG_FILE;
    sponge::Log::init(logfile);

    SPONGE_INFO("Starting game");

    maze = std::make_unique<Maze>();

    std::stringstream ss;
    ss << fmt::format("{} {} {}", game::project_name, game::project_version,
                      game::git_sha);
    std::string appName = ss.str();

    SPONGE_INFO("{}", appName);

#ifdef EMSCRIPTEN
    uint32_t width = 800;
    uint32_t height = 600;
#else
    uint32_t width = 1600;
    uint32_t height = 900;
#endif

    if (maze->construct(appName, width, height) == 0) {
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

bool shutdown() {
    SPONGE_INFO("Shutting down");

    sponge::Log::shutdown();

    return true;
}

extern "C" int main(int argc, char* args[]) {
    UNUSED(argc);
    UNUSED(args);

#ifdef EMSCRIPTEN
    startup();
#else
    startup();
#endif

    shutdown();

    return 0;
}
