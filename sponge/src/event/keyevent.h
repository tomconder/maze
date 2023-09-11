#pragma once

#include "core/keycode.h"
#include "event/event.h"

namespace sponge {

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

class KeyPressedEvent : public KeyEvent {
   public:
    explicit KeyPressedEvent(const KeyCode keycode, const uint32_t elapsedTime,
                             bool isRepeated = false)
        : KeyEvent(keycode), elapsedTime(elapsedTime), isRepeated(isRepeated) {}

    bool isHeld() const {
        return isRepeated;
    }

    uint32_t getElapsedTime() const {
        return elapsedTime;
    }

    EVENT_CLASS_TYPE(KeyPressed)

   private:
    bool isRepeated;
    uint32_t elapsedTime;
};

class KeyReleasedEvent : public KeyEvent {
   public:
    explicit KeyReleasedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent {
   public:
    explicit KeyTypedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

    EVENT_CLASS_TYPE(KeyTyped)
};

}  // namespace sponge
