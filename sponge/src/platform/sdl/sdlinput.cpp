#include "platform/sdl/sdlinput.h"

namespace Sponge {

Input* Input::instance = new SDLInput();

SDLInput::SDLInput() {
    initializeScanCodeMap();
}

bool SDLInput::isKeyPressedImpl(KeyCode key) {
    const Uint8* state = SDL_GetKeyboardState(nullptr);
    return state[mapKeyCodeToScanCode(key)];
}

bool SDLInput::isButtonPressedImpl() {
    Uint32 buttons = SDL_GetMouseState(nullptr, nullptr);
    return (buttons & SDL_BUTTON_LMASK) != 0;
}

void SDLInput::initializeScanCodeMap() {
    scanCodeMap[KeyCode::Space] = SDL_SCANCODE_SPACE;
    scanCodeMap[KeyCode::Apostrophe] = SDL_SCANCODE_APOSTROPHE;
    scanCodeMap[KeyCode::Comma] = SDL_SCANCODE_COMMA;
    scanCodeMap[KeyCode::Minus] = SDL_SCANCODE_MINUS;
    scanCodeMap[KeyCode::Period] = SDL_SCANCODE_PERIOD;
    scanCodeMap[KeyCode::Slash] = SDL_SCANCODE_SLASH;

    scanCodeMap[KeyCode::D0] = SDL_SCANCODE_0;
    scanCodeMap[KeyCode::D1] = SDL_SCANCODE_1;
    scanCodeMap[KeyCode::D2] = SDL_SCANCODE_2;
    scanCodeMap[KeyCode::D3] = SDL_SCANCODE_3;
    scanCodeMap[KeyCode::D4] = SDL_SCANCODE_4;
    scanCodeMap[KeyCode::D5] = SDL_SCANCODE_5;
    scanCodeMap[KeyCode::D6] = SDL_SCANCODE_6;
    scanCodeMap[KeyCode::D7] = SDL_SCANCODE_7;
    scanCodeMap[KeyCode::D8] = SDL_SCANCODE_8;
    scanCodeMap[KeyCode::D9] = SDL_SCANCODE_9;

    scanCodeMap[KeyCode::Semicolon] = SDL_SCANCODE_SEMICOLON;
    scanCodeMap[KeyCode::Equal] = SDL_SCANCODE_EQUALS;

    scanCodeMap[KeyCode::A] = SDL_SCANCODE_A;
    scanCodeMap[KeyCode::B] = SDL_SCANCODE_B;
    scanCodeMap[KeyCode::C] = SDL_SCANCODE_C;
    scanCodeMap[KeyCode::D] = SDL_SCANCODE_D;
    scanCodeMap[KeyCode::E] = SDL_SCANCODE_E;
    scanCodeMap[KeyCode::F] = SDL_SCANCODE_F;
    scanCodeMap[KeyCode::G] = SDL_SCANCODE_G;
    scanCodeMap[KeyCode::H] = SDL_SCANCODE_H;
    scanCodeMap[KeyCode::I] = SDL_SCANCODE_I;
    scanCodeMap[KeyCode::J] = SDL_SCANCODE_J;
    scanCodeMap[KeyCode::K] = SDL_SCANCODE_K;
    scanCodeMap[KeyCode::L] = SDL_SCANCODE_L;
    scanCodeMap[KeyCode::M] = SDL_SCANCODE_M;
    scanCodeMap[KeyCode::N] = SDL_SCANCODE_N;
    scanCodeMap[KeyCode::O] = SDL_SCANCODE_O;
    scanCodeMap[KeyCode::P] = SDL_SCANCODE_P;
    scanCodeMap[KeyCode::Q] = SDL_SCANCODE_Q;
    scanCodeMap[KeyCode::R] = SDL_SCANCODE_R;
    scanCodeMap[KeyCode::S] = SDL_SCANCODE_S;
    scanCodeMap[KeyCode::T] = SDL_SCANCODE_T;
    scanCodeMap[KeyCode::U] = SDL_SCANCODE_U;
    scanCodeMap[KeyCode::V] = SDL_SCANCODE_V;
    scanCodeMap[KeyCode::W] = SDL_SCANCODE_W;
    scanCodeMap[KeyCode::X] = SDL_SCANCODE_X;
    scanCodeMap[KeyCode::Y] = SDL_SCANCODE_Y;
    scanCodeMap[KeyCode::Z] = SDL_SCANCODE_Z;

    scanCodeMap[KeyCode::LeftBracket] = SDL_SCANCODE_LEFTBRACKET;
    scanCodeMap[KeyCode::Backslash] = SDL_SCANCODE_BACKSLASH;
    scanCodeMap[KeyCode::RightBracket] = SDL_SCANCODE_RIGHTBRACKET;
    scanCodeMap[KeyCode::GraveAccent] = SDL_SCANCODE_GRAVE;

    scanCodeMap[KeyCode::World1] = SDL_SCANCODE_INTERNATIONAL1;
    scanCodeMap[KeyCode::World2] = SDL_SCANCODE_INTERNATIONAL2;

    scanCodeMap[KeyCode::Escape] = SDL_SCANCODE_ESCAPE;
    scanCodeMap[KeyCode::Enter] = SDL_SCANCODE_RETURN;
    scanCodeMap[KeyCode::Tab] = SDL_SCANCODE_TAB;
    scanCodeMap[KeyCode::Backspace] = SDL_SCANCODE_BACKSPACE;
    scanCodeMap[KeyCode::Insert] = SDL_SCANCODE_INSERT;
    scanCodeMap[KeyCode::Delete] = SDL_SCANCODE_DELETE;
    scanCodeMap[KeyCode::Right] = SDL_SCANCODE_RIGHT;
    scanCodeMap[KeyCode::Left] = SDL_SCANCODE_LEFT;
    scanCodeMap[KeyCode::Down] = SDL_SCANCODE_DOWN;
    scanCodeMap[KeyCode::Up] = SDL_SCANCODE_UP;
    scanCodeMap[KeyCode::PageUp] = SDL_SCANCODE_PAGEUP;
    scanCodeMap[KeyCode::PageDown] = SDL_SCANCODE_PAGEDOWN;
    scanCodeMap[KeyCode::Home] = SDL_SCANCODE_HOME;
    scanCodeMap[KeyCode::End] = SDL_SCANCODE_END;
    scanCodeMap[KeyCode::CapsLock] = SDL_SCANCODE_CAPSLOCK;
    scanCodeMap[KeyCode::ScrollLock] = SDL_SCANCODE_SCROLLLOCK;
    scanCodeMap[KeyCode::NumLock] = SDL_SCANCODE_NUMLOCKCLEAR;
    scanCodeMap[KeyCode::PrintScreen] = SDL_SCANCODE_PRINTSCREEN;
    scanCodeMap[KeyCode::Pause] = SDL_SCANCODE_PAUSE;
    scanCodeMap[KeyCode::F1] = SDL_SCANCODE_F1;
    scanCodeMap[KeyCode::F2] = SDL_SCANCODE_F2;
    scanCodeMap[KeyCode::F3] = SDL_SCANCODE_F3;
    scanCodeMap[KeyCode::F4] = SDL_SCANCODE_F4;
    scanCodeMap[KeyCode::F5] = SDL_SCANCODE_F5;
    scanCodeMap[KeyCode::F6] = SDL_SCANCODE_F6;
    scanCodeMap[KeyCode::F7] = SDL_SCANCODE_F7;
    scanCodeMap[KeyCode::F8] = SDL_SCANCODE_F8;
    scanCodeMap[KeyCode::F9] = SDL_SCANCODE_F9;
    scanCodeMap[KeyCode::F10] = SDL_SCANCODE_F10;
    scanCodeMap[KeyCode::F11] = SDL_SCANCODE_F11;
    scanCodeMap[KeyCode::F12] = SDL_SCANCODE_F12;
    scanCodeMap[KeyCode::F13] = SDL_SCANCODE_F13;
    scanCodeMap[KeyCode::F14] = SDL_SCANCODE_F14;
    scanCodeMap[KeyCode::F15] = SDL_SCANCODE_F15;
    scanCodeMap[KeyCode::F16] = SDL_SCANCODE_F16;
    scanCodeMap[KeyCode::F17] = SDL_SCANCODE_F17;
    scanCodeMap[KeyCode::F18] = SDL_SCANCODE_F18;
    scanCodeMap[KeyCode::F19] = SDL_SCANCODE_F19;
    scanCodeMap[KeyCode::F20] = SDL_SCANCODE_F20;
    scanCodeMap[KeyCode::F21] = SDL_SCANCODE_F21;
    scanCodeMap[KeyCode::F22] = SDL_SCANCODE_F22;
    scanCodeMap[KeyCode::F23] = SDL_SCANCODE_F23;
    scanCodeMap[KeyCode::F24] = SDL_SCANCODE_F24;

    scanCodeMap[KeyCode::KP0] = SDL_SCANCODE_KP_0;
    scanCodeMap[KeyCode::KP1] = SDL_SCANCODE_KP_1;
    scanCodeMap[KeyCode::KP2] = SDL_SCANCODE_KP_2;
    scanCodeMap[KeyCode::KP3] = SDL_SCANCODE_KP_3;
    scanCodeMap[KeyCode::KP4] = SDL_SCANCODE_KP_4;
    scanCodeMap[KeyCode::KP5] = SDL_SCANCODE_KP_5;
    scanCodeMap[KeyCode::KP6] = SDL_SCANCODE_KP_6;
    scanCodeMap[KeyCode::KP7] = SDL_SCANCODE_KP_7;
    scanCodeMap[KeyCode::KP8] = SDL_SCANCODE_KP_8;
    scanCodeMap[KeyCode::KP9] = SDL_SCANCODE_KP_9;
    scanCodeMap[KeyCode::KPDecimal] = SDL_SCANCODE_KP_DECIMAL;
    scanCodeMap[KeyCode::KPDivide] = SDL_SCANCODE_KP_DIVIDE;
    scanCodeMap[KeyCode::KPMultiply] = SDL_SCANCODE_KP_MULTIPLY;
    scanCodeMap[KeyCode::KPSubtract] = SDL_SCANCODE_KP_MINUS;
    scanCodeMap[KeyCode::KPAdd] = SDL_SCANCODE_KP_PLUS;
    scanCodeMap[KeyCode::KPEnter] = SDL_SCANCODE_KP_ENTER;
    scanCodeMap[KeyCode::KPEqual] = SDL_SCANCODE_KP_EQUALS;

    scanCodeMap[KeyCode::LeftShift] = SDL_SCANCODE_LSHIFT;
    scanCodeMap[KeyCode::LeftControl] = SDL_SCANCODE_LCTRL;
    scanCodeMap[KeyCode::LeftAlt] = SDL_SCANCODE_LALT;
    scanCodeMap[KeyCode::LeftSuper] = SDL_SCANCODE_LGUI;
    scanCodeMap[KeyCode::RightShift] = SDL_SCANCODE_RSHIFT;
    scanCodeMap[KeyCode::RightControl] = SDL_SCANCODE_RCTRL;
    scanCodeMap[KeyCode::RightAlt] = SDL_SCANCODE_RALT;
    scanCodeMap[KeyCode::RightSuper] = SDL_SCANCODE_RGUI;
    scanCodeMap[KeyCode::Menu] = SDL_SCANCODE_MENU;
}

const SDL_Scancode& SDLInput::mapKeyCodeToScanCode(KeyCode key) {
    return scanCodeMap[key];
}

}  // namespace Sponge
