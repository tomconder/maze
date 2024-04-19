#include "sdlmouse.hpp"
#include <SDL.h>

namespace sponge::input {

bool SDLMouse::isButtonPressed() {
    const uint32_t buttons = SDL_GetMouseState(nullptr, nullptr);
    return (buttons & SDL_BUTTON_LMASK) == SDL_BUTTON(1);
}

MouseCode SDLMouse::mapMouseButton(uint8_t index) {
    return index - 1;
}

}  // namespace sponge::input
