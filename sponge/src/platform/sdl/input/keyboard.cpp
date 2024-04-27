#include "keyboard.hpp"
#include "keycodemap.hpp"
#include "scancodemap.hpp"

namespace sponge::platform::sdl::input {

using input::KeyCode;

Keyboard::Keyboard() {
    state = SDL_GetKeyboardState(nullptr);
}

bool Keyboard::isKeyPressed(const KeyCode key) {
    return state[mapKeyCodeToScanCode(key)] != 0;
}

SDL_Scancode& Keyboard::mapKeyCodeToScanCode(const KeyCode key) {
    return scanCodeMap[key];
}

KeyCode Keyboard::mapScanCodeToKeyCode(const SDL_Scancode& scancode) {
    const auto result = keyCodeMap.find(scancode);
    if (keyCodeMap.find(scancode) == keyCodeMap.end()) {
        return KeyCode::SpongeKey_None;
    }
    return result->second;
}

}  // namespace sponge::platform::sdl::input
