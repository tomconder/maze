#include "keyboard.hpp"
#include "codemap.hpp"

namespace sponge::platform::sdl::input {

using input::KeyCode;

Keyboard::Keyboard() {
    state = SDL_GetKeyboardState(nullptr);
}

bool Keyboard::isKeyPressed(const KeyCode key) const {
    return state[mapKeyCodeToScanCode(key)] != 0;
}

SDL_Scancode& Keyboard::mapKeyCodeToScanCode(const KeyCode key) {
    return scanCodeMap[key];
}

KeyCode Keyboard::mapScanCodeToKeyCode(const SDL_Scancode& scancode) {
    if (!keyCodeMap.contains(scancode)) {
        return KeyCode::SpongeKey_None;
    }
    const auto result = keyCodeMap.find(scancode);
    return result->second;
}

}  // namespace sponge::platform::sdl::input
