#include "platform/sdl/core/window.hpp"
#include "logging/log.hpp"

namespace sponge::platform::sdl {

Window::Window(const core::WindowProps& props) {
    init(props);
}

Window::~Window() noexcept {
    shutdown();
}

void Window::init(const core::WindowProps& props) {
    data.title = props.title;
    data.width = props.width;
    data.height = props.height;

    if (props.title.empty()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Sponge",
                                 "Title cannot be empty", nullptr);
        SPONGE_CORE_CRITICAL("Title cannot be empty");
    }

    if (!props.fullscreen) {
        SPONGE_CORE_INFO("Creating window {}x{}", props.width, props.height);
    }

    uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (props.fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    window = SDL_CreateWindow(
        props.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        static_cast<int>(props.width), static_cast<int>(props.height), flags);
    if (window == nullptr) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, props.title.c_str(),
                                 "Could not create window", nullptr);
        SPONGE_CORE_CRITICAL("Could not create window: {}", SDL_GetError());
    }

    int32_t w;
    int32_t h;
    SDL_GetWindowSize(window, &w, &h);
    data.width = static_cast<uint32_t>(w);
    data.height = static_cast<uint32_t>(h);
}

void Window::shutdown() const {
    if (window != nullptr) {
        SDL_DestroyWindow(window);
    }
}

}  // namespace sponge::platform::sdl
