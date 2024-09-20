#pragma once

#include "core/window.hpp"
#include <SDL.h>

namespace sponge::platform::sdl::core {

struct WindowData {
    std::string title;
    uint32_t width;
    uint32_t height;
};

class Window final : public sponge::core::Window {
   public:
    explicit Window(const sponge::core::WindowProps& props);
    ~Window() noexcept override;

    uint32_t getWidth() const override {
        return data.width;
    }

    uint32_t getHeight() const override {
        return data.height;
    }

    void* getNativeWindow() const override {
        return window;
    }

   private:
    void init(const sponge::core::WindowProps& props);
    void shutdown() const;

    WindowData data;
    SDL_Window* window = nullptr;
};

}  // namespace sponge::platform::sdl::core
