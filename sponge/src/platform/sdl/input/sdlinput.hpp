#pragma once

#include "input/input.hpp"
#include "input/keyboard/keycode.hpp"
#include <absl/container/flat_hash_map.h>
#include <SDL.h>

namespace sponge::input {

class SDLInput : public Input {
   public:
    SDLInput();

   protected:
    bool isKeyPressedImpl(keyboard::KeyCode key) override;
    bool isButtonPressedImpl() override;

   private:
    absl::flat_hash_map<keyboard::KeyCode, SDL_Scancode> scanCodeMap;

    void initializeScanCodeMap();
    const SDL_Scancode& mapKeyCodeToScanCode(keyboard::KeyCode key);

    const uint8_t* state;
};

}  // namespace sponge::input
