#pragma once

#include "input/keycode.hpp"
#include <absl/container/flat_hash_map.h>
#include <SDL.h>

namespace sponge::platform::sdl::input {

class Keyboard {
   public:
    Keyboard();

    bool isKeyPressed(sponge::input::KeyCode key);
    sponge::input::KeyCode mapScanCodeToKeyCode(const SDL_Scancode& scancode);

   private:
    absl::flat_hash_map<sponge::input::KeyCode, SDL_Scancode> scanCodeMap;
    absl::flat_hash_map<SDL_Scancode, sponge::input::KeyCode> keyCodeMap;

    void initializeScanCodeMap();
    void initializeKeyCodeMap();

    SDL_Scancode& mapKeyCodeToScanCode(sponge::input::KeyCode key);

    const uint8_t* state;
};

}  // namespace sponge::platform::sdl::input
