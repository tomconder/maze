#include "core/mousecode.h"
#include "event.h"

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
    float mouseX, mouseY;
};

class MouseScrolledEvent : public Event {
   public:
    MouseScrolledEvent(const float xOffset, const float yOffset) : xOffset(xOffset), yOffset(yOffset) {}

    float getXOffset() const {
        return xOffset;
    }
    float getYOffset() const {
        return yOffset;
    }

    EVENT_CLASS_TYPE(MouseScrolled)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
   private:
    float xOffset, yOffset;
};

class MouseButtonEvent : public Event {
   public:
    MouseCode getMouseButton() const {
        return button;
    }

    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)

   protected:
    MouseButtonEvent(const MouseCode button) : button(button) {}

    MouseCode button;
};

class MouseButtonPressedEvent : public MouseButtonEvent {
   public:
    MouseButtonPressedEvent(const MouseCode button) : MouseButtonEvent(button) {}

    EVENT_CLASS_TYPE(MouseButtonPressed)
};

class MouseButtonReleasedEvent : public MouseButtonEvent {
   public:
    MouseButtonReleasedEvent(const MouseCode button) : MouseButtonEvent(button) {}

    EVENT_CLASS_TYPE(MouseButtonReleased)
};
