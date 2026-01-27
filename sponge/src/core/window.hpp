#pragma once

#include <cstdint>
#include <string>

namespace sponge::core {

class WindowProps {
public:
    std::string_view title;
    uint32_t         width;
    uint32_t         height;
    bool             fullscreen;
};

class Window {
public:
    virtual ~Window() noexcept = default;

    virtual uint32_t getWidth() const   = 0;
    virtual uint32_t getHeight() const  = 0;
    virtual uint32_t getOffsetX() const = 0;
    virtual uint32_t getOffsetY() const = 0;

    virtual void* getNativeWindow() const = 0;
};

}  // namespace sponge::core
