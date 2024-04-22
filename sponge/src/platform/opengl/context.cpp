#include "context.hpp"
#include "core/log.hpp"
#include <tuplet/tuple.hpp>
#include <SDL.h>

namespace sponge::platform::opengl {

Context::Context(SDL_Window* window) {
    SPONGE_CORE_INFO("Initializing OpenGL");

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    constexpr auto glVersions =
        std::to_array<tuplet::tuple<int, int>>({ { 4, 6 },
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
                                                 { 2, 0 } });
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

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

Context::~Context() {
    SDL_GLContext context = SDL_GL_GetCurrentContext();
    SDL_GL_DeleteContext(context);
}

void Context::flip(void* window) {
    if (window == nullptr) {
        return;
    }
    SDL_GL_SwapWindow(static_cast<SDL_Window*>(window));
}

void Context::toggleFullscreen(void* window) {
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

}  // namespace sponge::platform::opengl
