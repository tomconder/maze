#pragma once

#include "input/input.hpp"
#include <absl/container/flat_hash_map.h>
#include <SDL.h>

namespace sponge {

class SDLInput : public input::Input {
   public:
    SDLInput();

   protected:
    bool isKeyPressedImpl(const input::KeyCode key) override;
    bool isButtonPressedImpl() override;

   private:
    absl::flat_hash_map<input::KeyCode, SDL_Scancode> scanCodeMap;

    void initializeScanCodeMap();
    const SDL_Scancode& mapKeyCodeToScanCode(const input::KeyCode key);

    const uint8_t* state;
};

}  // namespace sponge
