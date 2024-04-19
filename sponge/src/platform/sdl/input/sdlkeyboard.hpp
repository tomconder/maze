#pragma once

#include "input/keycode.hpp"
#include <absl/container/flat_hash_map.h>
#include <SDL.h>

namespace sponge::input {

class SDLKeyboard {
   public:
    SDLKeyboard();

    bool isKeyPressed(KeyCode key);
    KeyCode mapScanCodeToKeyCode(const SDL_Scancode& scancode);

   private:
    absl::flat_hash_map<KeyCode, SDL_Scancode> scanCodeMap;
    absl::flat_hash_map<SDL_Scancode, KeyCode> keyCodeMap;

    void initializeScanCodeMap();
    void initializeKeyCodeMap();

    SDL_Scancode& mapKeyCodeToScanCode(KeyCode key);

    const uint8_t* state;
};

}  // namespace sponge::input
