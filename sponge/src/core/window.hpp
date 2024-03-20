#pragma once

#include "core/base.hpp"

namespace sponge {

class WindowProps {
   public:
    std::string title;
    uint32_t width;
    uint32_t height;
};

class Window {
   public:
    virtual ~Window() noexcept = default;

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;

    virtual void setVSync(bool enabled) = 0;
    virtual bool isVSync() const = 0;

    virtual void* getNativeWindow() const = 0;
};

}  // namespace sponge
