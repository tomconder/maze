#pragma once

#include "event.h"

class WindowResizeEvent : public Event {
   public:
    WindowResizeEvent(uint32_t width, uint32_t height): width(width), height(height) {}

    uint32_t getWidth() { return width; }
    uint32_t getHeight() { return height; }

    EVENT_CLASS_TYPE(WindowResize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

   private:
    uint32_t width, height;
};

class WindowCloseEvent : public Event {
   public:
    WindowCloseEvent() = default;

    EVENT_CLASS_TYPE(WindowClose)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class WindowFullscreenEvent : public Event {
   public:
    WindowFullscreenEvent() = default;

    EVENT_CLASS_TYPE(WindowFullscreen)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};