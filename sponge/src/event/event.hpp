#pragma once

#include <cstdint>
#include <functional>

namespace sponge::event {
enum class EventType : uint8_t {
    None = 0,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled,
    WindowClose,
    WindowFocus,
    WindowResize,
};

#define EVENT_CLASS_TYPE(type)                \
    static EventType getStaticType() {        \
        return EventType::type;               \
    }                                         \
    EventType getEventType() const override { \
        return getStaticType();               \
    }

class Event {
public:
    virtual ~Event() = default;

    bool handled = false;

    virtual EventType getEventType() const = 0;
};

class EventDispatcher {
    template <typename T>
    using EventFn = std::function<bool(T&)>;

public:
    explicit EventDispatcher(Event& event) : event(event) {}

    template <typename T>
    bool dispatch(const EventFn<T>& func) {
        if (event.getEventType() == T::getStaticType() && !event.handled) {
            event.handled |= func(static_cast<T&>(event));
            return true;
        }
        return false;
    }

private:
    Event& event;
};
}  // namespace sponge::event
