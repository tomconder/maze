#pragma once
#include "input/mousecode.hpp"

namespace sponge::platform::sdl::input {

class Mouse {
   public:
    static bool isButtonPressed();
    static sponge::input::MouseCode mapMouseButton(uint8_t index);
};

}  // namespace sponge::platform::sdl::input
