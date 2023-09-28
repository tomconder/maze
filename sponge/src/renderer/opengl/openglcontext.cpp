#include "renderer/opengl/openglcontext.h"
#include <SDL.h>
#include <cassert>
#include <numeric>
#include <sstream>

#ifdef EMSCRIPTEN
#include <utility>
#endif

#include <spdlog/fmt/fmt.h>
#include <tuplet/tuple.hpp>
#include <array>
#include <utility>

namespace sponge {

OpenGLContext::OpenGLContext(SDL_Window* window, std::string name)
    : glName(std::move(name)) {
    SPONGE_CORE_INFO("Initializing OpenGL");

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

#ifdef EMSCRIPTEN
    constexpr std::array<tuplet::tuple<int, int>, 7> glVersions{
        { { 3, 2 }, { 3, 1 }, { 3, 0 }, { 2, 2 }, { 2, 1 }, { 2, 0 }, { 1, 1 } }
    };
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
#else
    constexpr std::array<tuplet::tuple<int, int>, 13> glVersions{ { { 4, 6 },
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
                                                                    { 2,
                                                                      0 } } };
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

    SPONGE_CORE_INFO("Creating OpenGL context");

    SDL_GLContext context = nullptr;

    // create context trying different versions
    for (const auto& [major, minor] : glVersions) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
        context = SDL_GL_CreateContext(window);
        if (context != nullptr) {
            break;
        }
    }

    if (context == nullptr) {
        SPONGE_CORE_ERROR("OpenGL context could not be created: {}",
                          SDL_GetError());
        return;
    }

    gladLoadGLLoader(static_cast<GLADloadproc>(SDL_GL_GetProcAddress));

    if (SDL_GL_MakeCurrent(window, context) < 0) {
        SPONGE_CORE_ERROR(
            "Could not be set up OpenGL context for rendering: {}",
            SDL_GetError());
        return;
    }

    if (window != nullptr) {
        SDL_GetWindowSize(window, &width, &height);
        glViewport(0, 0, width, height);
    }
}

OpenGLContext::~OpenGLContext() {
    SDL_GLContext context = SDL_GL_GetCurrentContext();
    SDL_GL_DeleteContext(context);
}

void OpenGLContext::flip(void* window) {
    if (window == nullptr) {
        return;
    }
    SDL_GL_SwapWindow(static_cast<SDL_Window*>(window));
}

void OpenGLContext::logGlVersion() const {
    auto minorVersion = 0;
    auto majorVersion = 0;
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);

    SPONGE_CORE_INFO("Created {} context: {}.{}", glName, majorVersion,
                     minorVersion);
}

void OpenGLContext::toggleFullscreen(void* window) {
    if (window == nullptr) {
        return;
    }

    isFullScreen = !isFullScreen;
    if (SDL_SetWindowFullscreen(
            static_cast<SDL_Window*>(window),
            isFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) < 0) {
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
        ss << fmt::format(
            "  #{}: {} {}", i, videoDriver,
            (currentVideoDriver == videoDriver ? "[current]" : ""));
        SPONGE_CORE_DEBUG(ss.str());
    }

    const int numRenderDrivers = SDL_GetNumRenderDrivers();
    SPONGE_CORE_DEBUG("Render Driver Info [{}]:", numRenderDrivers);

    SDL_RendererInfo info;
    for (int i = 0; i < numRenderDrivers; ++i) {
        SDL_GetRenderDriverInfo(i, &info);

        bool isSoftware = (info.flags & SDL_RENDERER_SOFTWARE) != 0U;
        bool isHardware = (info.flags & SDL_RENDERER_ACCELERATED) != 0U;
        bool isVSyncEnabled = (info.flags & SDL_RENDERER_PRESENTVSYNC) != 0U;
        bool isTargetTexture = (info.flags & SDL_RENDERER_TARGETTEXTURE) != 0U;

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

        auto flags =
            v.empty()
                ? ""
                : std::accumulate(++v.begin(), v.end(), *v.begin(),
                                  [](std::string a, std::string b) {
                                      return std::move(a) + ", " + std::move(b);
                                  });

        std::stringstream ss;

        ss << fmt::format("  #{}: {:10} [{}]", i, info.name, flags.c_str());
        SPONGE_CORE_DEBUG(ss.str());
    }
}

void OpenGLContext::logOpenGLContextInfo() {
    assert(SDL_GL_GetCurrentContext() && "Missing OpenGL Context");

    SPONGE_CORE_INFO("OpenGL Info:");

    std::stringstream ss;
    ss << fmt::format("  {:14} {}", "Version:",
                      reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    SPONGE_CORE_INFO(ss.str());

    ss.str("");
    ss << fmt::format("  {:14} {}", "GLSL:",
                      reinterpret_cast<const char*>(
                          glGetString(GL_SHADING_LANGUAGE_VERSION)));
    SPONGE_CORE_INFO(ss.str());

    ss.str("");
    ss << fmt::format("  {:14} {}", "Renderer:",
                      reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    SPONGE_CORE_INFO(ss.str());

    ss.str("");
    ss << fmt::format("  {:14} {}", "Vendor:",
                      reinterpret_cast<const char*>(glGetString(GL_VENDOR)));

    SPONGE_CORE_INFO(ss.str());

    GLint extensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensions);
    ss.str("");
    ss << fmt::format("  {:14} {}", "Extensions:", extensions);
    SPONGE_CORE_DEBUG(ss.str());
}

void OpenGLContext::logStaticOpenGLInfo() const {
#ifdef GL_GLEXT_VERSION
    SPONGE_CORE_DEBUG("OpenGL GLEXT version: {}", GL_GLEXT_VERSION);
#endif
}

}  // namespace sponge
