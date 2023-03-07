#include "renderer/opengl/openglcontext.h"

#include <SDL.h>

#include <cassert>
#include <numeric>
#include <sstream>

#ifdef EMSCRIPTEN
#include <utility>
#endif

#include <iomanip>
#include <utility>
#include <numeric>

namespace Sponge {

OpenGLContext::OpenGLContext(SDL_Window *window, std::string name) : glName(std::move(name)) {
    SPONGE_CORE_INFO("Initializing OpenGL");

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

#ifdef EMSCRIPTEN
    const std::array<std::tuple<int, int>, 7> glVersions{
        { { 3, 2 }, { 3, 1 }, { 3, 0 }, { 2, 2 }, { 2, 1 }, { 2, 0 }, { 1, 1 } }
    };
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
#else
    const std::array<std::tuple<int, int>, 13> glVersions{ { { 4, 6 },
                                                             { 4, 5 },
                                                             { 4, 4 },
                                                             { 4, 3 },
                                                             { 4, 2 },
                                                             { 4, 1 },
                                                             { 4, 0 },
                                                             { 3, 3 },
                                                             { 3, 2 },
                                                             { 3, 1 },
                                                             { 3, 0 },
                                                             { 2, 1 },
                                                             { 2, 0 } } };
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

    SPONGE_CORE_INFO("Creating OpenGL context");

    SDL_GLContext context = nullptr;

    // create context trying different versions
    for (const auto &[major, minor] : glVersions) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
        context = SDL_GL_CreateContext(window);
        if (context != nullptr) {
            break;
        }
    }

    if (context == nullptr) {
        SPONGE_CORE_ERROR("OpenGL context could not be created: {}", SDL_GetError());
        return;
    }

    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    if (SDL_GL_MakeCurrent(window, context) < 0) {
        SPONGE_CORE_ERROR("Could not be set up OpenGL context for rendering: {}", SDL_GetError());
        return;
    }

#ifdef EMSCRIPTEN
    setVSync(0);
#else
    setVSync(-1);
#endif

    this->window = window;

    if (window != nullptr) {
        SDL_GetWindowSize(window, &width, &height);
        glViewport(0, 0, width, height);
    }
}

OpenGLContext::~OpenGLContext() {
    SDL_GLContext context = SDL_GL_GetCurrentContext();
    SDL_GL_DeleteContext(context);
}

void OpenGLContext::flip() {
    if (window == nullptr) {
        return;
    }
    SDL_GL_SwapWindow(window);
}

void OpenGLContext::logGlVersion() const {
    auto minorVersion = 0;
    auto majorVersion = 0;
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);

    SPONGE_CORE_INFO("Created {} context: {}.{}", glName, majorVersion, minorVersion);
}

void OpenGLContext::toggleFullscreen() {
    isFullScreen = !isFullScreen;
    if (SDL_SetWindowFullscreen(window, isFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) < 0) {
        SPONGE_CORE_ERROR("Unable to toggle fullscreen: {}", SDL_GetError());
    }
}

void OpenGLContext::logGraphicsDriverInfo() {
    assert(SDL_GL_GetCurrentContext() && "Missing OpenGL Context");

    const auto numVideoDrivers = SDL_GetNumVideoDrivers();
    SPONGE_CORE_DEBUG("Video Driver Info [{}]:", numVideoDrivers);

    const std::string currentVideoDriver(SDL_GetCurrentVideoDriver());
    for (int i = 0; i < numVideoDrivers; i++) {
        const std::string videoDriver(SDL_GetVideoDriver(i));
        std::stringstream ss;
        ss << "  #" << std::setw(2) << i << ": " << videoDriver
           << (currentVideoDriver == videoDriver ? " (current)" : "");
        SPONGE_CORE_DEBUG(ss.str());
    }

    const int numRenderDrivers = SDL_GetNumRenderDrivers();
    SPONGE_CORE_DEBUG("Render Driver Info [{}]:", numRenderDrivers);

    SDL_RendererInfo info;
    for (int i = 0; i < numRenderDrivers; ++i) {
        SDL_GetRenderDriverInfo(i, &info);

        bool isSoftware = info.flags & SDL_RENDERER_SOFTWARE;
        bool isHardware = info.flags & SDL_RENDERER_ACCELERATED;
        bool isVSyncEnabled = info.flags & SDL_RENDERER_PRESENTVSYNC;
        bool isTargetTexture = info.flags & SDL_RENDERER_TARGETTEXTURE;

        std::vector<std::string> v;
        v.reserve(4);

        if (isSoftware) {
            v.emplace_back("SW");
        }
        if (isHardware) {
            v.emplace_back("HW");
        }
        if (isVSyncEnabled) {
            v.emplace_back("VSync");
        }
        if (isTargetTexture) {
            v.emplace_back("TTex");
        }

        auto flags = v.empty() ? "" : std::accumulate(++v.begin(), v.end(), *v.begin(), [](auto &a, auto &b) {
            return a.append(", ").append(b);
        });

        std::stringstream ss;
        ss << "  #" << std::setw(2) << i << ": " << std::setw(10) << std::left << info.name << " [" << flags.c_str()
           << "]";
        SPONGE_CORE_DEBUG(ss.str());
    }
}

void OpenGLContext::logOpenGLContextInfo() {
    assert(SDL_GL_GetCurrentContext() && "Missing OpenGL Context");

    SPONGE_CORE_INFO("OpenGL Info:");

    std::stringstream ss;
    ss << std::setw(20) << std::left << "  Version: " << glGetString(GL_VERSION);
    SPONGE_CORE_INFO(ss.str());

    ss.str("");
    ss << std::setw(20) << std::left << "  GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION);
    SPONGE_CORE_INFO(ss.str());

    ss.str("");
    ss << std::setw(20) << std::left << "  Renderer: " << glGetString(GL_RENDERER);
    SPONGE_CORE_INFO(ss.str());

    ss.str("");
    ss << std::setw(20) << std::left << "  Vendor: " << glGetString(GL_VENDOR);
    SPONGE_CORE_INFO(ss.str());

    GLint extensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensions);
    ss.str("");
    ss << std::setw(20) << std::left << "  Extensions: " << extensions;
    SPONGE_CORE_DEBUG(ss.str());
}

void OpenGLContext::logStaticOpenGLInfo() {
#ifdef GL_GLEXT_VERSION
    SPONGE_CORE_DEBUG("OpenGL GLEXT version: {}", GL_GLEXT_VERSION);
#endif
}

void OpenGLContext::setVSync(int interval) {
    // 0 for immediate updates
    // 1 for updates synchronized with the vertical retrace
    // -1 for adaptive vsync

    if (SDL_GL_SetSwapInterval(interval) == 0) {
        syncInterval = interval;
        SPONGE_CORE_DEBUG("Set vsync to {}", syncInterval);
        return;
    }

    //  the system does not support adaptive vsync, so try with vsync
    if (interval == -1 && SDL_GL_SetSwapInterval(1) == 0) {
        syncInterval = 1;
        SPONGE_CORE_DEBUG("Set vsync to {}", syncInterval);
        return;
    }

    SPONGE_CORE_ERROR("Unable to set vsync: {}", SDL_GetError());
}

}  // namespace Sponge
