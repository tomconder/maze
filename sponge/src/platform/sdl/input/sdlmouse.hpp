#pragma once
#include "input/mousecode.hpp"

namespace sponge::input {

class SDLMouse {
   public:
    static bool isButtonPressed();
    static MouseCode mapMouseButton(uint8_t index);
};

}  // namespace sponge::input
