#include "platform/sdl/sdlinput.h"

namespace sponge {

Input* Input::instance = new SDLInput();

SDLInput::SDLInput() {
    initializeScanCodeMap();
    state = SDL_GetKeyboardState(nullptr);
}

bool SDLInput::isKeyPressedImpl(KeyCode key) {
    return state[mapKeyCodeToScanCode(key)] != 0;
}

bool SDLInput::isButtonPressedImpl() {
    Uint32 buttons = SDL_GetMouseState(nullptr, nullptr);
    return (buttons & SDL_BUTTON_LMASK) != 0;
}

void SDLInput::initializeScanCodeMap() {
    scanCodeMap[KeyCode::SpongeKey_Space] = SDL_SCANCODE_SPACE;
    scanCodeMap[KeyCode::SpongeKey_Apostrophe] = SDL_SCANCODE_APOSTROPHE;
    scanCodeMap[KeyCode::SpongeKey_Comma] = SDL_SCANCODE_COMMA;
    scanCodeMap[KeyCode::SpongeKey_Minus] = SDL_SCANCODE_MINUS;
    scanCodeMap[KeyCode::SpongeKey_Period] = SDL_SCANCODE_PERIOD;
    scanCodeMap[KeyCode::SpongeKey_Slash] = SDL_SCANCODE_SLASH;

    scanCodeMap[KeyCode::SpongeKey_D0] = SDL_SCANCODE_0;
    scanCodeMap[KeyCode::SpongeKey_D1] = SDL_SCANCODE_1;
    scanCodeMap[KeyCode::SpongeKey_D2] = SDL_SCANCODE_2;
    scanCodeMap[KeyCode::SpongeKey_D3] = SDL_SCANCODE_3;
    scanCodeMap[KeyCode::SpongeKey_D4] = SDL_SCANCODE_4;
    scanCodeMap[KeyCode::SpongeKey_D5] = SDL_SCANCODE_5;
    scanCodeMap[KeyCode::SpongeKey_D6] = SDL_SCANCODE_6;
    scanCodeMap[KeyCode::SpongeKey_D7] = SDL_SCANCODE_7;
    scanCodeMap[KeyCode::SpongeKey_D8] = SDL_SCANCODE_8;
    scanCodeMap[KeyCode::SpongeKey_D9] = SDL_SCANCODE_9;

    scanCodeMap[KeyCode::SpongeKey_Semicolon] = SDL_SCANCODE_SEMICOLON;
    scanCodeMap[KeyCode::SpongeKey_Equal] = SDL_SCANCODE_EQUALS;

    scanCodeMap[KeyCode::SpongeKey_A] = SDL_SCANCODE_A;
    scanCodeMap[KeyCode::SpongeKey_B] = SDL_SCANCODE_B;
    scanCodeMap[KeyCode::SpongeKey_C] = SDL_SCANCODE_C;
    scanCodeMap[KeyCode::SpongeKey_D] = SDL_SCANCODE_D;
    scanCodeMap[KeyCode::SpongeKey_E] = SDL_SCANCODE_E;
    scanCodeMap[KeyCode::SpongeKey_F] = SDL_SCANCODE_F;
    scanCodeMap[KeyCode::SpongeKey_G] = SDL_SCANCODE_G;
    scanCodeMap[KeyCode::SpongeKey_H] = SDL_SCANCODE_H;
    scanCodeMap[KeyCode::SpongeKey_I] = SDL_SCANCODE_I;
    scanCodeMap[KeyCode::SpongeKey_J] = SDL_SCANCODE_J;
    scanCodeMap[KeyCode::SpongeKey_K] = SDL_SCANCODE_K;
    scanCodeMap[KeyCode::SpongeKey_L] = SDL_SCANCODE_L;
    scanCodeMap[KeyCode::SpongeKey_M] = SDL_SCANCODE_M;
    scanCodeMap[KeyCode::SpongeKey_N] = SDL_SCANCODE_N;
    scanCodeMap[KeyCode::SpongeKey_O] = SDL_SCANCODE_O;
    scanCodeMap[KeyCode::SpongeKey_P] = SDL_SCANCODE_P;
    scanCodeMap[KeyCode::SpongeKey_Q] = SDL_SCANCODE_Q;
    scanCodeMap[KeyCode::SpongeKey_R] = SDL_SCANCODE_R;
    scanCodeMap[KeyCode::SpongeKey_S] = SDL_SCANCODE_S;
    scanCodeMap[KeyCode::SpongeKey_T] = SDL_SCANCODE_T;
    scanCodeMap[KeyCode::SpongeKey_U] = SDL_SCANCODE_U;
    scanCodeMap[KeyCode::SpongeKey_V] = SDL_SCANCODE_V;
    scanCodeMap[KeyCode::SpongeKey_W] = SDL_SCANCODE_W;
    scanCodeMap[KeyCode::SpongeKey_X] = SDL_SCANCODE_X;
    scanCodeMap[KeyCode::SpongeKey_Y] = SDL_SCANCODE_Y;
    scanCodeMap[KeyCode::SpongeKey_Z] = SDL_SCANCODE_Z;

    scanCodeMap[KeyCode::SpongeKey_LeftBracket] = SDL_SCANCODE_LEFTBRACKET;
    scanCodeMap[KeyCode::SpongeKey_Backslash] = SDL_SCANCODE_BACKSLASH;
    scanCodeMap[KeyCode::SpongeKey_RightBracket] = SDL_SCANCODE_RIGHTBRACKET;
    scanCodeMap[KeyCode::SpongeKey_GraveAccent] = SDL_SCANCODE_GRAVE;

    scanCodeMap[KeyCode::SpongeKey_World1] = SDL_SCANCODE_INTERNATIONAL1;
    scanCodeMap[KeyCode::SpongeKey_World2] = SDL_SCANCODE_INTERNATIONAL2;

    scanCodeMap[KeyCode::SpongeKey_Escape] = SDL_SCANCODE_ESCAPE;
    scanCodeMap[KeyCode::SpongeKey_Enter] = SDL_SCANCODE_RETURN;
    scanCodeMap[KeyCode::SpongeKey_Tab] = SDL_SCANCODE_TAB;
    scanCodeMap[KeyCode::SpongeKey_Backspace] = SDL_SCANCODE_BACKSPACE;
    scanCodeMap[KeyCode::SpongeKey_Insert] = SDL_SCANCODE_INSERT;
    scanCodeMap[KeyCode::SpongeKey_Delete] = SDL_SCANCODE_DELETE;
    scanCodeMap[KeyCode::SpongeKey_Right] = SDL_SCANCODE_RIGHT;
    scanCodeMap[KeyCode::SpongeKey_Left] = SDL_SCANCODE_LEFT;
    scanCodeMap[KeyCode::SpongeKey_Down] = SDL_SCANCODE_DOWN;
    scanCodeMap[KeyCode::SpongeKey_Up] = SDL_SCANCODE_UP;
    scanCodeMap[KeyCode::SpongeKey_PageUp] = SDL_SCANCODE_PAGEUP;
    scanCodeMap[KeyCode::SpongeKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    scanCodeMap[KeyCode::SpongeKey_Home] = SDL_SCANCODE_HOME;
    scanCodeMap[KeyCode::SpongeKey_End] = SDL_SCANCODE_END;
    scanCodeMap[KeyCode::SpongeKey_CapsLock] = SDL_SCANCODE_CAPSLOCK;
    scanCodeMap[KeyCode::SpongeKey_ScrollLock] = SDL_SCANCODE_SCROLLLOCK;
    scanCodeMap[KeyCode::SpongeKey_NumLock] = SDL_SCANCODE_NUMLOCKCLEAR;
    scanCodeMap[KeyCode::SpongeKey_PrintScreen] = SDL_SCANCODE_PRINTSCREEN;
    scanCodeMap[KeyCode::SpongeKey_Pause] = SDL_SCANCODE_PAUSE;
    scanCodeMap[KeyCode::SpongeKey_F1] = SDL_SCANCODE_F1;
    scanCodeMap[KeyCode::SpongeKey_F2] = SDL_SCANCODE_F2;
    scanCodeMap[KeyCode::SpongeKey_F3] = SDL_SCANCODE_F3;
    scanCodeMap[KeyCode::SpongeKey_F4] = SDL_SCANCODE_F4;
    scanCodeMap[KeyCode::SpongeKey_F5] = SDL_SCANCODE_F5;
    scanCodeMap[KeyCode::SpongeKey_F6] = SDL_SCANCODE_F6;
    scanCodeMap[KeyCode::SpongeKey_F7] = SDL_SCANCODE_F7;
    scanCodeMap[KeyCode::SpongeKey_F8] = SDL_SCANCODE_F8;
    scanCodeMap[KeyCode::SpongeKey_F9] = SDL_SCANCODE_F9;
    scanCodeMap[KeyCode::SpongeKey_F10] = SDL_SCANCODE_F10;
    scanCodeMap[KeyCode::SpongeKey_F11] = SDL_SCANCODE_F11;
    scanCodeMap[KeyCode::SpongeKey_F12] = SDL_SCANCODE_F12;
    scanCodeMap[KeyCode::SpongeKey_F13] = SDL_SCANCODE_F13;
    scanCodeMap[KeyCode::SpongeKey_F14] = SDL_SCANCODE_F14;
    scanCodeMap[KeyCode::SpongeKey_F15] = SDL_SCANCODE_F15;
    scanCodeMap[KeyCode::SpongeKey_F16] = SDL_SCANCODE_F16;
    scanCodeMap[KeyCode::SpongeKey_F17] = SDL_SCANCODE_F17;
    scanCodeMap[KeyCode::SpongeKey_F18] = SDL_SCANCODE_F18;
    scanCodeMap[KeyCode::SpongeKey_F19] = SDL_SCANCODE_F19;
    scanCodeMap[KeyCode::SpongeKey_F20] = SDL_SCANCODE_F20;
    scanCodeMap[KeyCode::SpongeKey_F21] = SDL_SCANCODE_F21;
    scanCodeMap[KeyCode::SpongeKey_F22] = SDL_SCANCODE_F22;
    scanCodeMap[KeyCode::SpongeKey_F23] = SDL_SCANCODE_F23;
    scanCodeMap[KeyCode::SpongeKey_F24] = SDL_SCANCODE_F24;

    scanCodeMap[KeyCode::SpongeKey_KP0] = SDL_SCANCODE_KP_0;
    scanCodeMap[KeyCode::SpongeKey_KP1] = SDL_SCANCODE_KP_1;
    scanCodeMap[KeyCode::SpongeKey_KP2] = SDL_SCANCODE_KP_2;
    scanCodeMap[KeyCode::SpongeKey_KP3] = SDL_SCANCODE_KP_3;
    scanCodeMap[KeyCode::SpongeKey_KP4] = SDL_SCANCODE_KP_4;
    scanCodeMap[KeyCode::SpongeKey_KP5] = SDL_SCANCODE_KP_5;
    scanCodeMap[KeyCode::SpongeKey_KP6] = SDL_SCANCODE_KP_6;
    scanCodeMap[KeyCode::SpongeKey_KP7] = SDL_SCANCODE_KP_7;
    scanCodeMap[KeyCode::SpongeKey_KP8] = SDL_SCANCODE_KP_8;
    scanCodeMap[KeyCode::SpongeKey_KP9] = SDL_SCANCODE_KP_9;
    scanCodeMap[KeyCode::SpongeKey_KPDecimal] = SDL_SCANCODE_KP_DECIMAL;
    scanCodeMap[KeyCode::SpongeKey_KPDivide] = SDL_SCANCODE_KP_DIVIDE;
    scanCodeMap[KeyCode::SpongeKey_KPMultiply] = SDL_SCANCODE_KP_MULTIPLY;
    scanCodeMap[KeyCode::SpongeKey_KPSubtract] = SDL_SCANCODE_KP_MINUS;
    scanCodeMap[KeyCode::SpongeKey_KPAdd] = SDL_SCANCODE_KP_PLUS;
    scanCodeMap[KeyCode::SpongeKey_KPEnter] = SDL_SCANCODE_KP_ENTER;
    scanCodeMap[KeyCode::SpongeKey_KPEqual] = SDL_SCANCODE_KP_EQUALS;

    scanCodeMap[KeyCode::SpongeKey_LeftShift] = SDL_SCANCODE_LSHIFT;
    scanCodeMap[KeyCode::SpongeKey_LeftControl] = SDL_SCANCODE_LCTRL;
    scanCodeMap[KeyCode::SpongeKey_LeftAlt] = SDL_SCANCODE_LALT;
    scanCodeMap[KeyCode::SpongeKey_LeftSuper] = SDL_SCANCODE_LGUI;
    scanCodeMap[KeyCode::SpongeKey_RightShift] = SDL_SCANCODE_RSHIFT;
    scanCodeMap[KeyCode::SpongeKey_RightControl] = SDL_SCANCODE_RCTRL;
    scanCodeMap[KeyCode::SpongeKey_RightAlt] = SDL_SCANCODE_RALT;
    scanCodeMap[KeyCode::SpongeKey_RightSuper] = SDL_SCANCODE_RGUI;
    scanCodeMap[KeyCode::SpongeKey_Menu] = SDL_SCANCODE_MENU;
}

const SDL_Scancode& SDLInput::mapKeyCodeToScanCode(KeyCode key) {
    return scanCodeMap[key];
}

}  // namespace sponge
