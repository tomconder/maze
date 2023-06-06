#pragma once

#include "core/mousecode.h"
#include "event/event.h"

namespace sponge {

class MouseMovedEvent : public Event {
   public:
    MouseMovedEvent(const float x, const float y) : mouseX(x), mouseY(y) {}

    float getX() const {
        return mouseX;
    }
    float getY() const {
        return mouseY;
    }

    EVENT_CLASS_TYPE(MouseMoved)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

   private:
    float mouseX;
    float mouseY;
};

class MouseScrolledEvent : public Event {
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
    MouseCode getMouseButton() const {
        return button;
    }

    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput |
                         EventCategoryMouseButton)

   protected:
    explicit MouseButtonEvent(const MouseCode button) : button(button) {}

   private:
    MouseCode button;
};

class MouseButtonPressedEvent : public MouseButtonEvent {
   public:
    explicit MouseButtonPressedEvent(const MouseCode button)
        : MouseButtonEvent(button) {}

    EVENT_CLASS_TYPE(MouseButtonPressed)
};

class MouseButtonReleasedEvent : public MouseButtonEvent {
   public:
    explicit MouseButtonReleasedEvent(const MouseCode button)
        : MouseButtonEvent(button) {}

    EVENT_CLASS_TYPE(MouseButtonReleased)
};

}  // namespace sponge
