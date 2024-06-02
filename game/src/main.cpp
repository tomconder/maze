#include "maze.hpp"
#include "sponge.hpp"
#include "version.h"

namespace maze {
bool main(int argc, char* argv[]) {
    UNUSED(argc);
    UNUSED(argv);

    constexpr uint32_t width = 1600;
    constexpr uint32_t height = 900;

    auto spec =
        sponge::platform::sdl::ApplicationSpecification{ .name =
                                                             game::project_name,
                                                         .width = width,
                                                         .height = height,
                                                         .fullscreen = true };

    auto maze = game::Maze{ spec };

    SPONGE_INFO("Starting game: {} {} ({})", game::project_name,
                game::project_version, game::git_sha);

    if (!maze.start()) {
        return false;
    }

    SPONGE_INFO("Iterating loop");

    bool quit = false;
    while (!quit) {
        quit = maze.iterateLoop();
    }

    SPONGE_INFO("Shutting down");
    maze.shutdown();

    return true;
}

}  // namespace maze

extern "C" int main(int argc, char* argv[]) {
    sponge::startupCore();
    maze::main(argc, argv);
    sponge::shutdownCore();

    return 0;
}
