#pragma once

#include "core/keycode.hpp"
#include "event/event.hpp"

namespace sponge::event {

class KeyEvent : public Event {
   public:
    KeyCode getKeyCode() const {
        return keyCode;
    }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

   protected:
    explicit KeyEvent(const KeyCode keyCode) : keyCode(keyCode) {}

   private:
    KeyCode keyCode;
};

class KeyPressedEvent final : public KeyEvent {
   public:
    explicit KeyPressedEvent(const KeyCode keycode, const double elapsedTime,
                             const bool isRepeated = false)
        : KeyEvent(keycode), elapsedTime(elapsedTime), isRepeated(isRepeated) {}

    bool isHeld() const {
        return isRepeated;
    }

    double getElapsedTime() const {
        return elapsedTime;
    }

    EVENT_CLASS_TYPE(KeyPressed)

   private:
    bool isRepeated;
    double elapsedTime;
};

class KeyReleasedEvent final : public KeyEvent {
   public:
    explicit KeyReleasedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent {
   public:
    explicit KeyTypedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

    EVENT_CLASS_TYPE(KeyTyped)
};

}  // namespace sponge::event
