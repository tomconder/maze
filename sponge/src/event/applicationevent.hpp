#pragma once

#include "event/event.hpp"

#include <sstream>
#include <string>

namespace sponge::event {

class WindowResizeEvent final : public Event {
public:
    WindowResizeEvent(const uint32_t width, const uint32_t height) :
        width(width), height(height) {}

    uint32_t getWidth() const {
        return width;
    }
    uint32_t getHeight() const {
        return height;
    }

    std::string toString() const override {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << width << ", " << height;
        return ss.str();
    }

    EVENT_CLASS_TYPE(WindowResize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

private:
    uint32_t width;
    uint32_t height;
};

class WindowCloseEvent final : public Event {
public:
    WindowCloseEvent() = default;

    EVENT_CLASS_TYPE(WindowClose)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class WindowFullscreenEvent final : public Event {
public:
    WindowFullscreenEvent() = default;

    EVENT_CLASS_TYPE(WindowFullscreen)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class WindowMinimizeEvent final : public Event {
public:
    explicit WindowMinimizeEvent(const bool minimized) : minimized(minimized) {}

    bool isMinimized() const {
        return minimized;
    }

    EVENT_CLASS_TYPE(WindowMinimize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

private:
    bool minimized;
};

}  // namespace sponge::event
