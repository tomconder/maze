#pragma once

#include "core/window.h"
#include <SDL.h>

namespace sponge {

struct WindowData {
    std::string title;
    uint32_t width;
    uint32_t height;
    bool vsync;
};

class SDLWindow : public Window {
   public:
    explicit SDLWindow(const WindowProps& props);
    ~SDLWindow() noexcept override;

    uint32_t getWidth() const override {
        return data.width;
    }

    uint32_t getHeight() const override {
        return data.height;
    };

    void setVSync(bool enabled) override;

    bool isVSync() const override;

    void* getNativeWindow() const override {
        return window;
    }

   private:
    void init(const WindowProps& props);
    void shutdown();

    WindowData data;
    SDL_Window* window = nullptr;
};

}  // namespace sponge
