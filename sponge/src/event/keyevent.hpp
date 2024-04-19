#pragma once

#include "event/event.hpp"
#include "input/keyboard/keycode.hpp"

namespace sponge::event {

class KeyEvent : public Event {
   public:
    input::keyboard::KeyCode getKeyCode() const {
        return keyCode;
    }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

   protected:
    explicit KeyEvent(const input::keyboard::KeyCode keyCode)
        : keyCode(keyCode) {}

   private:
    input::keyboard::KeyCode keyCode;
};

class KeyPressedEvent final : public KeyEvent {
   public:
    explicit KeyPressedEvent(const input::keyboard::KeyCode keycode,
                             const double elapsedTime,
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
    explicit KeyReleasedEvent(const input::keyboard::KeyCode keycode)
        : KeyEvent(keycode) {}

    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent {
   public:
    explicit KeyTypedEvent(const input::keyboard::KeyCode keycode)
        : KeyEvent(keycode) {}

    EVENT_CLASS_TYPE(KeyTyped)
};

}  // namespace sponge::event
