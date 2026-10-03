#pragma once

#include "core/base.hpp"
#include "platform/glfw/core/application.hpp"
#include <memory>

extern std::unique_ptr<sponge::platform::glfw::core::Application>
    sponge::platform::glfw::core::createApplication(int argc, char** argv);

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4005)  // macro redefinition
#include "windows.h"
#pragma warning(pop)
#endif

namespace sponge::core {

int main(const int argc, char** argv) {
    startupCore();

    const auto app = platform::glfw::core::createApplication(argc, argv);
    app->run();

    shutdownCore();

    return 0;
}
}  // namespace sponge::core

int main(const int argc, char* argv[]) {
    return sponge::core::main(argc, argv);
}

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return sponge::core::main(__argc, __argv);
}
#endif
