#pragma once

#include "core/base.h"

namespace sponge {

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

#define BIND_EVENT_FN(fn)                                       \
    [this](auto&&... args) -> decltype(auto) {                  \
        return this->fn(std::forward<decltype(args)>(args)...); \
    }

#define EVENT_CLASS_TYPE(type)                \
    static EventType getStaticType() {        \
        return EventType::type;               \
    }                                         \
    EventType getEventType() const override { \
        return getStaticType();               \
    }

#define EVENT_CLASS_CATEGORY(category)      \
    int getCategoryFlags() const override { \
        return category;                    \
    }

class Event {
   public:
    virtual ~Event() = default;

    bool handled = false;

    virtual EventType getEventType() const = 0;
    virtual int getCategoryFlags() const = 0;

    bool isInCategory(EventCategory category) const {
        return (getCategoryFlags() & category) != 0;
    }
};

class EventDispatcher {
   public:
    explicit EventDispatcher(Event& event) : event(event){};

    template <typename T, typename F>
    bool dispatch(const F& func) {
        if (event.getEventType() == T::getStaticType()) {
            event.handled |= func(static_cast<T&>(event));
            return true;
        }
        return false;
    }

   private:
    Event& event;
};

}  // namespace sponge
