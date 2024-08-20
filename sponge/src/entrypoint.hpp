#pragma once

#include "core/application.hpp"

extern sponge::Application* sponge::createApplication(int argc, char** argv);

namespace sponge {

int main(const int argc, char** argv) {
    startupCore();
    Application* app = createApplication(argc, argv);
    app->run();
    delete app;
    shutdownCore();

    return 0;
}
}  // namespace sponge

extern "C" int main(const int argc, char* argv[]) {
    return sponge::main(argc, argv);
}
