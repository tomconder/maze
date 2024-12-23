#pragma once

#include "core/application.hpp"
#ifdef _WIN32
#include "windows.h"
#endif

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

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nShowCmd) {
    UNUSED(hInstance);
    UNUSED(hPrevInstance);
    UNUSED(lpCmdLine);
    UNUSED(nShowCmd);
    return sponge::core::main(__argc, __argv);
}
#endif
