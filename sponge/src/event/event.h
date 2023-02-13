#pragma once

#include "core/base.h"

enum class EventType {
    None = 0,
    WindowClose,
    WindowResize,
    WindowFullscreen,
    KeyPressed,
    KeyReleased,
    KeyTyped,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled
};

enum EventCategory {
    None = 0,
    EventCategoryApplication = BIT(0),
    EventCategoryInput = BIT(1),
    EventCategoryKeyboard = BIT(2),
    EventCategoryMouse = BIT(3),
    EventCategoryMouseButton = BIT(4)
};

#define EVENT_CLASS_TYPE(type)                                   \
    static EventType getStaticType() { return EventType::type; } \
    virtual EventType getEventType() const override { return getStaticType(); }

#define EVENT_CLASS_CATEGORY(category) \
    virtual int getCategoryFlags() const override { return category; }

class Event {
   public:
    virtual ~Event() = default;

    bool handled = false;

    virtual EventType getEventType() const = 0;
    virtual int getCategoryFlags() const = 0;

    bool isInCategory(EventCategory category) {
        return getCategoryFlags() & category;
    }
};
