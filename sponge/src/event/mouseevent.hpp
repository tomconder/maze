#pragma once

#include "event/event.hpp"
#include "input/mousecode.hpp"

namespace sponge::event {

class MouseMovedEvent final : public Event {
   public:
    MouseMovedEvent(const float xrel, const float yrel, const float xpos,
                    const float ypos)
        : xRelative(xrel), yRelative(yrel), x(xpos), y(ypos) {}

    float getXRelative() const {
        return xRelative;
    }
    float getYRelative() const {
        return yRelative;
    }
    float getX() const {
        return x;
    }
    float getY() const {
        return y;
    }

    EVENT_CLASS_TYPE(MouseMoved)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

   private:
    float xRelative;
    float yRelative;
    float x;
    float y;
};

class MouseScrolledEvent final : public Event {
   public:
    MouseScrolledEvent(const float xOffset, const float yOffset)
        : xOffset(xOffset), yOffset(yOffset) {}

    float getXOffset() const {
        return xOffset;
    }
    float getYOffset() const {
        return yOffset;
    }

    EVENT_CLASS_TYPE(MouseScrolled)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

   private:
    float xOffset;
    float yOffset;
};

class MouseButtonEvent : public Event {
   public:
    input::MouseButton getMouseButton() const {
        return button;
    }

    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput |
                         EventCategoryMouseButton)

   protected:
    explicit MouseButtonEvent(const input::MouseButton button)
        : button(button) {}

   private:
    input::MouseButton button;
};

class MouseButtonPressedEvent final : public MouseButtonEvent {
   public:
    explicit MouseButtonPressedEvent(const input::MouseButton button)
        : MouseButtonEvent(button) {}

    EVENT_CLASS_TYPE(MouseButtonPressed)
};

class MouseButtonReleasedEvent final : public MouseButtonEvent {
   public:
    explicit MouseButtonReleasedEvent(const input::MouseButton button)
        : MouseButtonEvent(button) {}

    EVENT_CLASS_TYPE(MouseButtonReleased)
};

class MouseButtonDownEvent final : public MouseButtonEvent {
   public:
    explicit MouseButtonDownEvent(const input::MouseButton button)
        : MouseButtonEvent(button) {}

    EVENT_CLASS_TYPE(MouseButtonDown)
};

}  // namespace sponge::event
