#include "keyboard.hpp"

namespace sponge::platform::sdl::input {

Keyboard::Keyboard() {
    initializeScanCodeMap();
    initializeKeyCodeMap();
    state = SDL_GetKeyboardState(nullptr);
}

bool Keyboard::isKeyPressed(const sponge::input::KeyCode key) {
    return state[mapKeyCodeToScanCode(key)] != 0;
}

void Keyboard::initializeScanCodeMap() {
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Space] = SDL_SCANCODE_SPACE;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Apostrophe] =
        SDL_SCANCODE_APOSTROPHE;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Comma] = SDL_SCANCODE_COMMA;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Minus] = SDL_SCANCODE_MINUS;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Period] = SDL_SCANCODE_PERIOD;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Slash] = SDL_SCANCODE_SLASH;

    scanCodeMap[sponge::input::KeyCode::SpongeKey_D0] = SDL_SCANCODE_0;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_D1] = SDL_SCANCODE_1;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_D2] = SDL_SCANCODE_2;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_D3] = SDL_SCANCODE_3;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_D4] = SDL_SCANCODE_4;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_D5] = SDL_SCANCODE_5;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_D6] = SDL_SCANCODE_6;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_D7] = SDL_SCANCODE_7;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_D8] = SDL_SCANCODE_8;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_D9] = SDL_SCANCODE_9;

    scanCodeMap[sponge::input::KeyCode::SpongeKey_Semicolon] =
        SDL_SCANCODE_SEMICOLON;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Equal] = SDL_SCANCODE_EQUALS;

    scanCodeMap[sponge::input::KeyCode::SpongeKey_A] = SDL_SCANCODE_A;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_B] = SDL_SCANCODE_B;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_C] = SDL_SCANCODE_C;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_D] = SDL_SCANCODE_D;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_E] = SDL_SCANCODE_E;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F] = SDL_SCANCODE_F;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_G] = SDL_SCANCODE_G;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_H] = SDL_SCANCODE_H;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_I] = SDL_SCANCODE_I;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_J] = SDL_SCANCODE_J;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_K] = SDL_SCANCODE_K;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_L] = SDL_SCANCODE_L;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_M] = SDL_SCANCODE_M;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_N] = SDL_SCANCODE_N;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_O] = SDL_SCANCODE_O;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_P] = SDL_SCANCODE_P;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Q] = SDL_SCANCODE_Q;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_R] = SDL_SCANCODE_R;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_S] = SDL_SCANCODE_S;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_T] = SDL_SCANCODE_T;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_U] = SDL_SCANCODE_U;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_V] = SDL_SCANCODE_V;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_W] = SDL_SCANCODE_W;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_X] = SDL_SCANCODE_X;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Y] = SDL_SCANCODE_Y;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Z] = SDL_SCANCODE_Z;

    scanCodeMap[sponge::input::KeyCode::SpongeKey_LeftBracket] =
        SDL_SCANCODE_LEFTBRACKET;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Backslash] =
        SDL_SCANCODE_BACKSLASH;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_RightBracket] =
        SDL_SCANCODE_RIGHTBRACKET;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_GraveAccent] =
        SDL_SCANCODE_GRAVE;

    scanCodeMap[sponge::input::KeyCode::SpongeKey_World1] =
        SDL_SCANCODE_INTERNATIONAL1;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_World2] =
        SDL_SCANCODE_INTERNATIONAL2;

    scanCodeMap[sponge::input::KeyCode::SpongeKey_Escape] = SDL_SCANCODE_ESCAPE;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Enter] = SDL_SCANCODE_RETURN;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Tab] = SDL_SCANCODE_TAB;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Backspace] =
        SDL_SCANCODE_BACKSPACE;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Insert] = SDL_SCANCODE_INSERT;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Delete] = SDL_SCANCODE_DELETE;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Right] = SDL_SCANCODE_RIGHT;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Left] = SDL_SCANCODE_LEFT;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Down] = SDL_SCANCODE_DOWN;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Up] = SDL_SCANCODE_UP;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_PageUp] = SDL_SCANCODE_PAGEUP;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_PageDown] =
        SDL_SCANCODE_PAGEDOWN;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Home] = SDL_SCANCODE_HOME;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_End] = SDL_SCANCODE_END;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_CapsLock] =
        SDL_SCANCODE_CAPSLOCK;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_ScrollLock] =
        SDL_SCANCODE_SCROLLLOCK;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_NumLock] =
        SDL_SCANCODE_NUMLOCKCLEAR;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_PrintScreen] =
        SDL_SCANCODE_PRINTSCREEN;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Pause] = SDL_SCANCODE_PAUSE;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F1] = SDL_SCANCODE_F1;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F2] = SDL_SCANCODE_F2;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F3] = SDL_SCANCODE_F3;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F4] = SDL_SCANCODE_F4;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F5] = SDL_SCANCODE_F5;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F6] = SDL_SCANCODE_F6;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F7] = SDL_SCANCODE_F7;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F8] = SDL_SCANCODE_F8;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F9] = SDL_SCANCODE_F9;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F10] = SDL_SCANCODE_F10;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F11] = SDL_SCANCODE_F11;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F12] = SDL_SCANCODE_F12;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F13] = SDL_SCANCODE_F13;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F14] = SDL_SCANCODE_F14;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F15] = SDL_SCANCODE_F15;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F16] = SDL_SCANCODE_F16;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F17] = SDL_SCANCODE_F17;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F18] = SDL_SCANCODE_F18;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F19] = SDL_SCANCODE_F19;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F20] = SDL_SCANCODE_F20;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F21] = SDL_SCANCODE_F21;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F22] = SDL_SCANCODE_F22;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F23] = SDL_SCANCODE_F23;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_F24] = SDL_SCANCODE_F24;

    scanCodeMap[sponge::input::KeyCode::SpongeKey_KP0] = SDL_SCANCODE_KP_0;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KP1] = SDL_SCANCODE_KP_1;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KP2] = SDL_SCANCODE_KP_2;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KP3] = SDL_SCANCODE_KP_3;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KP4] = SDL_SCANCODE_KP_4;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KP5] = SDL_SCANCODE_KP_5;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KP6] = SDL_SCANCODE_KP_6;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KP7] = SDL_SCANCODE_KP_7;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KP8] = SDL_SCANCODE_KP_8;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KP9] = SDL_SCANCODE_KP_9;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KPDecimal] =
        SDL_SCANCODE_KP_DECIMAL;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KPDivide] =
        SDL_SCANCODE_KP_DIVIDE;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KPMultiply] =
        SDL_SCANCODE_KP_MULTIPLY;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KPSubtract] =
        SDL_SCANCODE_KP_MINUS;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KPAdd] = SDL_SCANCODE_KP_PLUS;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KPEnter] =
        SDL_SCANCODE_KP_ENTER;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_KPEqual] =
        SDL_SCANCODE_KP_EQUALS;

    scanCodeMap[sponge::input::KeyCode::SpongeKey_LeftShift] =
        SDL_SCANCODE_LSHIFT;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_LeftControl] =
        SDL_SCANCODE_LCTRL;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_LeftAlt] = SDL_SCANCODE_LALT;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_LeftSuper] =
        SDL_SCANCODE_LGUI;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_RightShift] =
        SDL_SCANCODE_RSHIFT;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_RightControl] =
        SDL_SCANCODE_RCTRL;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_RightAlt] = SDL_SCANCODE_RALT;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_RightSuper] =
        SDL_SCANCODE_RGUI;
    scanCodeMap[sponge::input::KeyCode::SpongeKey_Menu] = SDL_SCANCODE_MENU;
}

SDL_Scancode& Keyboard::mapKeyCodeToScanCode(const sponge::input::KeyCode key) {
    return scanCodeMap[key];
}

void Keyboard::initializeKeyCodeMap() {
    keyCodeMap[SDL_SCANCODE_SPACE] = sponge::input::KeyCode::SpongeKey_Space;
    keyCodeMap[SDL_SCANCODE_APOSTROPHE] =
        sponge::input::KeyCode::SpongeKey_Apostrophe;
    keyCodeMap[SDL_SCANCODE_COMMA] = sponge::input::KeyCode::SpongeKey_Comma;
    keyCodeMap[SDL_SCANCODE_MINUS] = sponge::input::KeyCode::SpongeKey_Minus;
    keyCodeMap[SDL_SCANCODE_PERIOD] = sponge::input::KeyCode::SpongeKey_Period;
    keyCodeMap[SDL_SCANCODE_SLASH] = sponge::input::KeyCode::SpongeKey_Slash;

    keyCodeMap[SDL_SCANCODE_0] = sponge::input::KeyCode::SpongeKey_D0;
    keyCodeMap[SDL_SCANCODE_1] = sponge::input::KeyCode::SpongeKey_D1;
    keyCodeMap[SDL_SCANCODE_2] = sponge::input::KeyCode::SpongeKey_D2;
    keyCodeMap[SDL_SCANCODE_3] = sponge::input::KeyCode::SpongeKey_D3;
    keyCodeMap[SDL_SCANCODE_4] = sponge::input::KeyCode::SpongeKey_D4;
    keyCodeMap[SDL_SCANCODE_5] = sponge::input::KeyCode::SpongeKey_D5;
    keyCodeMap[SDL_SCANCODE_6] = sponge::input::KeyCode::SpongeKey_D6;
    keyCodeMap[SDL_SCANCODE_7] = sponge::input::KeyCode::SpongeKey_D7;
    keyCodeMap[SDL_SCANCODE_8] = sponge::input::KeyCode::SpongeKey_D8;
    keyCodeMap[SDL_SCANCODE_9] = sponge::input::KeyCode::SpongeKey_D9;

    keyCodeMap[SDL_SCANCODE_SEMICOLON] =
        sponge::input::KeyCode::SpongeKey_Semicolon;
    keyCodeMap[SDL_SCANCODE_EQUALS] = sponge::input::KeyCode::SpongeKey_Equal;

    keyCodeMap[SDL_SCANCODE_A] = sponge::input::KeyCode::SpongeKey_A;
    keyCodeMap[SDL_SCANCODE_B] = sponge::input::KeyCode::SpongeKey_B;
    keyCodeMap[SDL_SCANCODE_C] = sponge::input::KeyCode::SpongeKey_C;
    keyCodeMap[SDL_SCANCODE_D] = sponge::input::KeyCode::SpongeKey_D;
    keyCodeMap[SDL_SCANCODE_E] = sponge::input::KeyCode::SpongeKey_E;
    keyCodeMap[SDL_SCANCODE_F] = sponge::input::KeyCode::SpongeKey_F;
    keyCodeMap[SDL_SCANCODE_G] = sponge::input::KeyCode::SpongeKey_G;
    keyCodeMap[SDL_SCANCODE_H] = sponge::input::KeyCode::SpongeKey_H;
    keyCodeMap[SDL_SCANCODE_I] = sponge::input::KeyCode::SpongeKey_I;
    keyCodeMap[SDL_SCANCODE_J] = sponge::input::KeyCode::SpongeKey_J;
    keyCodeMap[SDL_SCANCODE_K] = sponge::input::KeyCode::SpongeKey_K;
    keyCodeMap[SDL_SCANCODE_L] = sponge::input::KeyCode::SpongeKey_L;
    keyCodeMap[SDL_SCANCODE_M] = sponge::input::KeyCode::SpongeKey_M;
    keyCodeMap[SDL_SCANCODE_N] = sponge::input::KeyCode::SpongeKey_N;
    keyCodeMap[SDL_SCANCODE_O] = sponge::input::KeyCode::SpongeKey_O;
    keyCodeMap[SDL_SCANCODE_P] = sponge::input::KeyCode::SpongeKey_P;
    keyCodeMap[SDL_SCANCODE_Q] = sponge::input::KeyCode::SpongeKey_Q;
    keyCodeMap[SDL_SCANCODE_R] = sponge::input::KeyCode::SpongeKey_R;
    keyCodeMap[SDL_SCANCODE_S] = sponge::input::KeyCode::SpongeKey_S;
    keyCodeMap[SDL_SCANCODE_T] = sponge::input::KeyCode::SpongeKey_T;
    keyCodeMap[SDL_SCANCODE_U] = sponge::input::KeyCode::SpongeKey_U;
    keyCodeMap[SDL_SCANCODE_V] = sponge::input::KeyCode::SpongeKey_V;
    keyCodeMap[SDL_SCANCODE_W] = sponge::input::KeyCode::SpongeKey_W;
    keyCodeMap[SDL_SCANCODE_X] = sponge::input::KeyCode::SpongeKey_X;
    keyCodeMap[SDL_SCANCODE_Y] = sponge::input::KeyCode::SpongeKey_Y;
    keyCodeMap[SDL_SCANCODE_Z] = sponge::input::KeyCode::SpongeKey_Z;

    keyCodeMap[SDL_SCANCODE_LEFTBRACKET] =
        sponge::input::KeyCode::SpongeKey_LeftBracket;
    keyCodeMap[SDL_SCANCODE_BACKSLASH] =
        sponge::input::KeyCode::SpongeKey_Backslash;
    keyCodeMap[SDL_SCANCODE_RIGHTBRACKET] =
        sponge::input::KeyCode::SpongeKey_RightBracket;
    keyCodeMap[SDL_SCANCODE_GRAVE] =
        sponge::input::KeyCode::SpongeKey_GraveAccent;

    keyCodeMap[SDL_SCANCODE_INTERNATIONAL1] =
        sponge::input::KeyCode::SpongeKey_World1;
    keyCodeMap[SDL_SCANCODE_INTERNATIONAL2] =
        sponge::input::KeyCode::SpongeKey_World2;

    keyCodeMap[SDL_SCANCODE_ESCAPE] = sponge::input::KeyCode::SpongeKey_Escape;
    keyCodeMap[SDL_SCANCODE_RETURN] = sponge::input::KeyCode::SpongeKey_Enter;
    keyCodeMap[SDL_SCANCODE_TAB] = sponge::input::KeyCode::SpongeKey_Tab;
    keyCodeMap[SDL_SCANCODE_BACKSPACE] =
        sponge::input::KeyCode::SpongeKey_Backspace;
    keyCodeMap[SDL_SCANCODE_INSERT] = sponge::input::KeyCode::SpongeKey_Insert;
    keyCodeMap[SDL_SCANCODE_DELETE] = sponge::input::KeyCode::SpongeKey_Delete;
    keyCodeMap[SDL_SCANCODE_RIGHT] = sponge::input::KeyCode::SpongeKey_Right;
    keyCodeMap[SDL_SCANCODE_LEFT] = sponge::input::KeyCode::SpongeKey_Left;
    keyCodeMap[SDL_SCANCODE_DOWN] = sponge::input::KeyCode::SpongeKey_Down;
    keyCodeMap[SDL_SCANCODE_UP] = sponge::input::KeyCode::SpongeKey_Up;
    keyCodeMap[SDL_SCANCODE_PAGEUP] = sponge::input::KeyCode::SpongeKey_PageUp;
    keyCodeMap[SDL_SCANCODE_PAGEDOWN] =
        sponge::input::KeyCode::SpongeKey_PageDown;
    keyCodeMap[SDL_SCANCODE_HOME] = sponge::input::KeyCode::SpongeKey_Home;
    keyCodeMap[SDL_SCANCODE_END] = sponge::input::KeyCode::SpongeKey_End;
    keyCodeMap[SDL_SCANCODE_CAPSLOCK] =
        sponge::input::KeyCode::SpongeKey_CapsLock;
    keyCodeMap[SDL_SCANCODE_SCROLLLOCK] =
        sponge::input::KeyCode::SpongeKey_ScrollLock;
    keyCodeMap[SDL_SCANCODE_NUMLOCKCLEAR] =
        sponge::input::KeyCode::SpongeKey_NumLock;
    keyCodeMap[SDL_SCANCODE_PRINTSCREEN] =
        sponge::input::KeyCode::SpongeKey_PrintScreen;
    keyCodeMap[SDL_SCANCODE_PAUSE] = sponge::input::KeyCode::SpongeKey_Pause;
    keyCodeMap[SDL_SCANCODE_F1] = sponge::input::KeyCode::SpongeKey_F1;
    keyCodeMap[SDL_SCANCODE_F2] = sponge::input::KeyCode::SpongeKey_F2;
    keyCodeMap[SDL_SCANCODE_F3] = sponge::input::KeyCode::SpongeKey_F3;
    keyCodeMap[SDL_SCANCODE_F4] = sponge::input::KeyCode::SpongeKey_F4;
    keyCodeMap[SDL_SCANCODE_F5] = sponge::input::KeyCode::SpongeKey_F5;
    keyCodeMap[SDL_SCANCODE_F6] = sponge::input::KeyCode::SpongeKey_F6;
    keyCodeMap[SDL_SCANCODE_F7] = sponge::input::KeyCode::SpongeKey_F7;
    keyCodeMap[SDL_SCANCODE_F8] = sponge::input::KeyCode::SpongeKey_F8;
    keyCodeMap[SDL_SCANCODE_F9] = sponge::input::KeyCode::SpongeKey_F9;
    keyCodeMap[SDL_SCANCODE_F10] = sponge::input::KeyCode::SpongeKey_F10;
    keyCodeMap[SDL_SCANCODE_F11] = sponge::input::KeyCode::SpongeKey_F11;
    keyCodeMap[SDL_SCANCODE_F12] = sponge::input::KeyCode::SpongeKey_F12;
    keyCodeMap[SDL_SCANCODE_F13] = sponge::input::KeyCode::SpongeKey_F13;
    keyCodeMap[SDL_SCANCODE_F14] = sponge::input::KeyCode::SpongeKey_F14;
    keyCodeMap[SDL_SCANCODE_F15] = sponge::input::KeyCode::SpongeKey_F15;
    keyCodeMap[SDL_SCANCODE_F16] = sponge::input::KeyCode::SpongeKey_F16;
    keyCodeMap[SDL_SCANCODE_F17] = sponge::input::KeyCode::SpongeKey_F17;
    keyCodeMap[SDL_SCANCODE_F18] = sponge::input::KeyCode::SpongeKey_F18;
    keyCodeMap[SDL_SCANCODE_F19] = sponge::input::KeyCode::SpongeKey_F19;
    keyCodeMap[SDL_SCANCODE_F20] = sponge::input::KeyCode::SpongeKey_F20;
    keyCodeMap[SDL_SCANCODE_F21] = sponge::input::KeyCode::SpongeKey_F21;
    keyCodeMap[SDL_SCANCODE_F22] = sponge::input::KeyCode::SpongeKey_F22;
    keyCodeMap[SDL_SCANCODE_F23] = sponge::input::KeyCode::SpongeKey_F23;
    keyCodeMap[SDL_SCANCODE_F24] = sponge::input::KeyCode::SpongeKey_F24;

    keyCodeMap[SDL_SCANCODE_KP_0] = sponge::input::KeyCode::SpongeKey_KP0;
    keyCodeMap[SDL_SCANCODE_KP_1] = sponge::input::KeyCode::SpongeKey_KP1;
    keyCodeMap[SDL_SCANCODE_KP_2] = sponge::input::KeyCode::SpongeKey_KP2;
    keyCodeMap[SDL_SCANCODE_KP_3] = sponge::input::KeyCode::SpongeKey_KP3;
    keyCodeMap[SDL_SCANCODE_KP_4] = sponge::input::KeyCode::SpongeKey_KP4;
    keyCodeMap[SDL_SCANCODE_KP_5] = sponge::input::KeyCode::SpongeKey_KP5;
    keyCodeMap[SDL_SCANCODE_KP_6] = sponge::input::KeyCode::SpongeKey_KP6;
    keyCodeMap[SDL_SCANCODE_KP_7] = sponge::input::KeyCode::SpongeKey_KP7;
    keyCodeMap[SDL_SCANCODE_KP_8] = sponge::input::KeyCode::SpongeKey_KP8;
    keyCodeMap[SDL_SCANCODE_KP_9] = sponge::input::KeyCode::SpongeKey_KP9;
    keyCodeMap[SDL_SCANCODE_KP_DECIMAL] =
        sponge::input::KeyCode::SpongeKey_KPDecimal;
    keyCodeMap[SDL_SCANCODE_KP_DIVIDE] =
        sponge::input::KeyCode::SpongeKey_KPDivide;
    keyCodeMap[SDL_SCANCODE_KP_MULTIPLY] =
        sponge::input::KeyCode::SpongeKey_KPMultiply;
    keyCodeMap[SDL_SCANCODE_KP_MINUS] =
        sponge::input::KeyCode::SpongeKey_KPSubtract;
    keyCodeMap[SDL_SCANCODE_KP_PLUS] = sponge::input::KeyCode::SpongeKey_KPAdd;
    keyCodeMap[SDL_SCANCODE_KP_ENTER] =
        sponge::input::KeyCode::SpongeKey_KPEnter;
    keyCodeMap[SDL_SCANCODE_KP_EQUALS] =
        sponge::input::KeyCode::SpongeKey_KPEqual;

    keyCodeMap[SDL_SCANCODE_LSHIFT] =
        sponge::input::KeyCode::SpongeKey_LeftShift;
    keyCodeMap[SDL_SCANCODE_LCTRL] =
        sponge::input::KeyCode::SpongeKey_LeftControl;
    keyCodeMap[SDL_SCANCODE_LALT] = sponge::input::KeyCode::SpongeKey_LeftAlt;
    keyCodeMap[SDL_SCANCODE_LGUI] = sponge::input::KeyCode::SpongeKey_LeftSuper;
    keyCodeMap[SDL_SCANCODE_RSHIFT] =
        sponge::input::KeyCode::SpongeKey_RightShift;
    keyCodeMap[SDL_SCANCODE_RCTRL] =
        sponge::input::KeyCode::SpongeKey_RightControl;
    keyCodeMap[SDL_SCANCODE_RALT] = sponge::input::KeyCode::SpongeKey_RightAlt;
    keyCodeMap[SDL_SCANCODE_RGUI] =
        sponge::input::KeyCode::SpongeKey_RightSuper;
    keyCodeMap[SDL_SCANCODE_MENU] = sponge::input::KeyCode::SpongeKey_Menu;
}

sponge::input::KeyCode Keyboard::mapScanCodeToKeyCode(
    const SDL_Scancode& scancode) {
    const auto result = keyCodeMap.find(scancode);
    if (keyCodeMap.find(scancode) == keyCodeMap.end()) {
        return sponge::input::KeyCode::SpongeKey_None;
    }
    return result->second;
}

}  // namespace sponge::platform::sdl::input
