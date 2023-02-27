#pragma once

#include "core/keycode.h"
#include "event/event.h"

namespace Sponge {

class KeyEvent : public Event {
   public:
    KeyCode getKeyCode() const {
        return keyCode;
    }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

   protected:
    KeyEvent(const KeyCode keyCode) : keyCode(keyCode) {}

    KeyCode keyCode;
};

class KeyPressedEvent : public KeyEvent {
   public:
    KeyPressedEvent(const KeyCode keycode, bool isRepeated = false) : KeyEvent(keycode), isRepeated(isRepeated) {}

    bool isHeld() const {
        return isRepeated;
    }

    EVENT_CLASS_TYPE(KeyPressed)

   private:
    bool isRepeated;
};

class KeyReleasedEvent : public KeyEvent {
   public:
    KeyReleasedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent {
   public:
    KeyTypedEvent(const KeyCode keycode) : KeyEvent(keycode) {}

    EVENT_CLASS_TYPE(KeyTyped)
};

}  // namespace Sponge
