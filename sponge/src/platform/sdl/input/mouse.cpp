#include "mouse.hpp"
#include <SDL.h>

namespace sponge::platform::sdl::input {

bool Mouse::isButtonPressed() {
    const uint32_t buttons = SDL_GetMouseState(nullptr, nullptr);
    return (buttons & SDL_BUTTON_LMASK) == SDL_BUTTON(1);
}

sponge::input::MouseCode Mouse::mapMouseButton(const uint8_t index) {
    return index - 1;
}

}  // namespace sponge::platform::sdl::input
