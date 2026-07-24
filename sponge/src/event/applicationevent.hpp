#pragma once

#include "event/event.hpp"

#include <cstdint>

namespace sponge::event {

class WindowResizeEvent final : public Event {
public:
    explicit WindowResizeEvent(const uint32_t width, const uint32_t height) :
        width(width), height(height) {}

    uint32_t getWidth() const {
        return width;
    }
    uint32_t getHeight() const {
        return height;
    }

    EVENT_CLASS_TYPE(WindowResize)

private:
    uint32_t width;
    uint32_t height;
};

class WindowCloseEvent final : public Event {
public:
    WindowCloseEvent() = default;

    EVENT_CLASS_TYPE(WindowClose)
};

class WindowFocusEvent final : public Event {
public:
    explicit WindowFocusEvent(const bool focused) : focused(focused) {}

    bool isFocused() const {
        return focused;
    }

    EVENT_CLASS_TYPE(WindowFocus)

private:
    bool focused;
};

class WindowFullscreenEvent final : public Event {
public:
    WindowFullscreenEvent() = default;

    EVENT_CLASS_TYPE(WindowFullscreen)
};

class WindowMinimizeEvent final : public Event {
public:
    explicit WindowMinimizeEvent(const bool minimized) : minimized(minimized) {}

    bool isMinimized() const {
        return minimized;
    }

    EVENT_CLASS_TYPE(WindowMinimize)

private:
    bool minimized;
};

}  // namespace sponge::event
