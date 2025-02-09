#pragma once

#include "core/base.hpp"
#include <cstdint>
#include <functional>
#include <ostream>
#include <string>

namespace sponge::event {
enum class EventType : uint8_t {
    None = 0,
    KeyPressed,
    KeyReleased,
    KeyTyped,
    MouseButtonDown,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled,
    WindowClose,
    WindowFullscreen,
    WindowMinimize,
    WindowResize
};

enum EventCategory : uint8_t {
    None = 0,
    EventCategoryApplication = BIT(0),
    EventCategoryInput = BIT(1),
    EventCategoryKeyboard = BIT(2),
    EventCategoryMouse = BIT(3),
    EventCategoryMouseButton = BIT(4)
};

#define EVENT_CLASS_TYPE(type)                        \
    static EventType getStaticType() {                \
        return EventType::type;                       \
    }                                                 \
    virtual EventType getEventType() const override { \
        return getStaticType();                       \
    }                                                 \
    virtual const char* getName() const override {    \
        return #type;                                 \
    }

#define EVENT_CLASS_CATEGORY(category)      \
    int getCategoryFlags() const override { \
        return category;                    \
    }

class Event {
public:
    virtual ~Event() = default;

    virtual std::string toString() const {
        return getName();
    }

    bool handled = false;

    virtual EventType getEventType() const = 0;
    virtual int getCategoryFlags() const = 0;
    virtual const char* getName() const = 0;

    bool isInCategory(const EventCategory category) const {
        return (getCategoryFlags() & category) != 0;
    }
};

class EventDispatcher {
    template <typename T>
    using EventFn = std::function<bool(T&)>;

public:
    explicit EventDispatcher(Event& event)
        : event(event) {
    }

    template <typename T>
    bool dispatch(EventFn<T> func) {
        if (event.getEventType() == T::getStaticType() && !event.handled) {
            event.handled |= func(static_cast<T&>(event));
            return true;
        }
        return false;
    }

private:
    Event& event;
};

inline std::ostream& operator<<(std::ostream& os, const Event& e) {
    return os << e.toString();
}
} // namespace sponge::event
