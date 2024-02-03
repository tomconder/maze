
#include "maze.h"
#include "sponge.h"
#include "version.h"

constexpr std::string_view spongeLogFile = "log.txt";

bool startup() {
    const auto maze = std::make_unique<Maze>();

    const auto logfile = sponge::File::getLogDir(game::project_name.data()) +
                         spongeLogFile.data();
    sponge::Log::init(logfile);

    SPONGE_INFO("Starting game");

    std::stringstream ss;
    ss << fmt::format("{}", game::project_name);
    std::string appName = ss.str();

    SPONGE_INFO("{}", appName);

    constexpr uint32_t width = 1600;
    constexpr uint32_t height = 900;

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
    sponge::Log::shutdown();

    return true;
}

extern "C" int main(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    startup();

    shutdown();

    return 0;
}
