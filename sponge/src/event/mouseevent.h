#pragma once

#include "core/mousecode.h"
#include "event/event.h"

namespace sponge {

class MouseMovedEvent : public Event {
   public:
    MouseMovedEvent(const float xrel, const float yrel, float xpos, float ypos)
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
    MouseButtonPressedEvent(const MouseCode button, float xpos, float ypos)
        : MouseButtonEvent(button), x(xpos), y(ypos) {}

    float getX() const {
        return x;
    }
    float getY() const {
        return y;
    }

    EVENT_CLASS_TYPE(MouseButtonPressed)

   private:
    float x;
    float y;
};

class MouseButtonReleasedEvent : public MouseButtonEvent {
   public:
    explicit MouseButtonReleasedEvent(const MouseCode button)
        : MouseButtonEvent(button) {}

    EVENT_CLASS_TYPE(MouseButtonReleased)
};

}  // namespace sponge
