#include "platform/sdl/sdlwindow.h"

namespace Sponge {

SDLWindow::SDLWindow(const WindowProps& props) {
    init(props);
}

SDLWindow::~SDLWindow() noexcept {
    shutdown();
}

void SDLWindow::init(const WindowProps& props) {
    data.title = props.title;
    data.width = props.width;
    data.height = props.height;

    if (props.title.empty()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Sponge", "Title cannot be empty", nullptr);
        SPONGE_CORE_CRITICAL("Title cannot be empty");
    }

    SPONGE_CORE_INFO("Creating window {} {}x{}", props.title, props.width, props.height);

    window = SDL_CreateWindow(props.title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              static_cast<int>(props.width), static_cast<int>(props.height),
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    if (window == nullptr) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, props.title.c_str(), "Could not create window", nullptr);
        SPONGE_CORE_CRITICAL("Could not create window: {}", SDL_GetError());
    }
}

void SDLWindow::shutdown() {
    if (window != nullptr) {
        SDL_DestroyWindow(window);
    }
}

void SDLWindow::setVSync(bool enabled) {
    // 0 for immediate updates
    // 1 for updates synchronized with the vertical retrace
    // -1 for adaptive vsync

    if (int interval = enabled ? 1 : 0; SDL_GL_SetSwapInterval(interval) == 0) {
        data.vsync = enabled;
        SPONGE_CORE_DEBUG("Set vsync to {}", enabled);
        return;
    }

    SPONGE_CORE_ERROR("Unable to set vsync: {}", SDL_GetError());
}

bool SDLWindow::isVSync() const {
    return data.vsync;
}

}  // namespace Sponge