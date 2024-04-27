#pragma once

#include "input/keycode.hpp"
#include <SDL.h>

namespace sponge::platform::sdl::input {

using sponge::input::KeyCode;

class Keyboard {
   public:
    Keyboard();

    bool isKeyPressed(KeyCode key);
    static KeyCode mapScanCodeToKeyCode(const SDL_Scancode& scancode);

   private:
    static SDL_Scancode& mapKeyCodeToScanCode(KeyCode key);

    const uint8_t* state;
};

}  // namespace sponge::platform::sdl::input
