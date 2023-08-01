#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include "maze.h"
#include "sponge.h"
#include "version.h"

#if __APPLE__
#include <glob.h>
#include <sysdir.h>
#endif

std::unique_ptr<Maze> maze;

// loop iteration is broken out like this for emscripten
bool iterateLoop() {
    return maze->iterateLoop();
}

#if __APPLE__
std::string expandTilde(const char* str) {
    if (!str)
        return {};

    glob_t globbuf;
    if (glob(str, GLOB_TILDE, nullptr, &globbuf) == 0) {
        std::string result(globbuf.gl_pathv[0]);
        globfree(&globbuf);
        return result;
    } else {
        throw std::runtime_error("Unable to expand tilde");
    }
}

std::string settingsPath() {
    char path[PATH_MAX];
    auto state = sysdir_start_search_path_enumeration(
        SYSDIR_DIRECTORY_APPLICATION_SUPPORT, SYSDIR_DOMAIN_MASK_USER);
    if ((state = sysdir_get_next_search_path_enumeration(state, path))) {
        return expandTilde(path);
    } else {
        throw std::runtime_error("Failed to get settings folder");
    }
}
#endif

bool startup() {
#ifdef __APPLE__
    std::string logfile = settingsPath() + "/maze/" + SPONGE_LOG_FILE;
#else
    std::string logfile = SPONGE_LOG_FILE;
#endif

    sponge::Log::init(logfile);

    SPONGE_INFO("Starting game");

    maze = std::make_unique<Maze>();

    std::string appName = "Maze ";
    appName += MAZE_VERSION;

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
