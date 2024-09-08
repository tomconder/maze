#pragma once

#include "event/event.hpp"
#include "input/keycode.hpp"

namespace sponge::event {

class KeyEvent : public Event {
   public:
    input::KeyCode getKeyCode() const {
        return keyCode;
    }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

   protected:
    explicit KeyEvent(const input::KeyCode keyCode) : keyCode(keyCode) {}

   private:
    input::KeyCode keyCode;
};

class KeyPressedEvent final : public KeyEvent {
   public:
    explicit KeyPressedEvent(const input::KeyCode keycode,
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
    double elapsedTime;
    bool isRepeated;
};

class KeyReleasedEvent final : public KeyEvent {
   public:
    explicit KeyReleasedEvent(const input::KeyCode keycode)
        : KeyEvent(keycode) {}

    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent final : public KeyEvent {
   public:
    explicit KeyTypedEvent(const input::KeyCode keycode) : KeyEvent(keycode) {}

    EVENT_CLASS_TYPE(KeyTyped)
};

}  // namespace sponge::event
