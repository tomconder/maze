#include "maze.hpp"
#include "sponge.hpp"
#include "version.h"

namespace maze {
bool main(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    const auto maze = std::make_unique<game::Maze>();

    SPONGE_INFO("Starting game: {} {} ({})", game::project_name, game::project_version, game::git_sha);

    std::stringstream ss;
    ss << fmt::format("{}", game::project_name);
    std::string appName = ss.str();

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

    SPONGE_INFO("Shutting down");
    maze->shutdown();

    return true;
}

}  // namespace maze

extern "C" int main(int argc, char *argv[]) {
    sponge::startupCore();
    maze::main(argc, argv);
    sponge::shutdownCore();

    return 0;
}
