#pragma once

#include "core/base.hpp"
#include <string>

namespace sponge {

class WindowProps {
   public:
    std::string title;
    uint32_t width;
    uint32_t height;
    bool fullscreen;
};

class Window {
   public:
    virtual ~Window() noexcept = default;

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;

    virtual void* getNativeWindow() const = 0;
};

}  // namespace sponge
