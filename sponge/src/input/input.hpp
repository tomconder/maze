#pragma once

#include "input/keyboard/keycode.hpp"

namespace sponge::input {

class Input {
   public:
    virtual ~Input() = default;

    static bool isKeyPressed(keyboard::KeyCode key) {
        return instance->isKeyPressedImpl(key);
    }

    static bool isButtonPressed() {
        return instance->isButtonPressedImpl();
    }

   protected:
    virtual bool isKeyPressedImpl(keyboard::KeyCode key) = 0;
    virtual bool isButtonPressedImpl() = 0;

   private:
    static Input* instance;
};

}  // namespace sponge::input
