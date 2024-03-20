#pragma once

#include "keycode.hpp"

namespace sponge::input {

class Input {
   public:
    virtual ~Input() = default;

    static bool isKeyPressed(KeyCode key) {
        return instance->isKeyPressedImpl(key);
    }

    static bool isButtonPressed() {
        return instance->isButtonPressedImpl();
    }

   protected:
    virtual bool isKeyPressedImpl(KeyCode key) = 0;
    virtual bool isButtonPressedImpl() = 0;

   private:
    static Input* instance;
};

}  // namespace sponge::input
