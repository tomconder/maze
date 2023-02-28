#pragma once

#include <SDL.h>

#include "core/input.h"

namespace Sponge {

class SDLInput : public Input {
   public:
    SDLInput();

   protected:
    bool isKeyPressedImpl(KeyCode key) override;
    bool isButtonPressedImpl() override;

   private:
    std::unordered_map<KeyCode, SDL_Scancode> scanCodeMap;

    void initializeScanCodeMap();
    const SDL_Scancode& mapKeyCodeToScanCode(KeyCode key);
};

}  // namespace Sponge
