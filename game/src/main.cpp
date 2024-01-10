
#include "maze.h"
#include "sponge.h"
#include "version.h"

constexpr std::string_view spongeLogFile = "log.txt";

bool startup() {
    const std::unique_ptr<Maze> maze = std::make_unique<Maze>();

    auto logfile = sponge::File::getLogDir() + spongeLogFile.data();
    sponge::Log::init(logfile);

    SPONGE_INFO("Starting game");

    std::stringstream ss;
    ss << fmt::format("{}", game::project_name);
    std::string appName = ss.str();

    SPONGE_INFO("{}", appName);

    uint32_t width = 1600;
    uint32_t height = 900;

    if (!maze->construct(appName, width, height)) {
        return false;
    }

    if (!maze->start()) {
        return false;
    }

    SPONGE_INFO("Iterating loop");

    bool quit = false;
    while (!quit) {
        quit = maze->iterateLoop();
    }

    maze->shutdown();

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

    startup();

    shutdown();

    return 0;
}
