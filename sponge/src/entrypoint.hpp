#pragma once

#include "core/application.hpp"

extern sponge::core::Application* sponge::core::createApplication(int argc,
                                                                  char** argv);

namespace sponge::core {

int main(const int argc, char** argv) {
    startupCore();
    Application* app = createApplication(argc, argv);
    app->run();
    delete app;
    shutdownCore();

    return 0;
}
}  // namespace sponge::core

extern "C" int main(const int argc, char* argv[]) {
    return sponge::core::main(argc, argv);
}
