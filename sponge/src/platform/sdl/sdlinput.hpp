#pragma once

#include "core/input.hpp"
#include <absl/container/flat_hash_map.h>
#include <SDL.h>

namespace sponge {

class SDLInput : public Input {
   public:
    SDLInput();

   protected:
    bool isKeyPressedImpl(const KeyCode key) override;
    bool isButtonPressedImpl() override;

   private:
    absl::flat_hash_map<KeyCode, SDL_Scancode> scanCodeMap;

    void initializeScanCodeMap();
    const SDL_Scancode& mapKeyCodeToScanCode(const KeyCode key);

    const uint8_t* state;
};

}  // namespace sponge
