#include "sdlinput.h"
#include "core/keycode.h"
#include "core/mousecode.h"

SDLInput::SDLInput() {
    initializeKeyMap();
}

void SDLInput::initializeKeyMap() {
    keymap[SDL_SCANCODE_SPACE] = KeyCode::Space;
    keymap[SDL_SCANCODE_APOSTROPHE] = KeyCode::Apostrophe;
    keymap[SDL_SCANCODE_COMMA] = KeyCode::Comma;
    keymap[SDL_SCANCODE_MINUS] = KeyCode::Minus;
    keymap[SDL_SCANCODE_PERIOD] = KeyCode::Period;
    keymap[SDL_SCANCODE_SLASH] = KeyCode::Slash;

    keymap[SDL_SCANCODE_0] = KeyCode::D0;
    keymap[SDL_SCANCODE_1] = KeyCode::D1;
    keymap[SDL_SCANCODE_2] = KeyCode::D2;
    keymap[SDL_SCANCODE_3] = KeyCode::D3;
    keymap[SDL_SCANCODE_4] = KeyCode::D4;
    keymap[SDL_SCANCODE_5] = KeyCode::D5;
    keymap[SDL_SCANCODE_6] = KeyCode::D6;
    keymap[SDL_SCANCODE_7] = KeyCode::D7;
    keymap[SDL_SCANCODE_8] = KeyCode::D8;
    keymap[SDL_SCANCODE_9] = KeyCode::D9;

    keymap[SDL_SCANCODE_SEMICOLON] = KeyCode::Semicolon;
    keymap[SDL_SCANCODE_EQUALS] = KeyCode::Equal;

    keymap[SDL_SCANCODE_A] = KeyCode::A;
    keymap[SDL_SCANCODE_B] = KeyCode::B;
    keymap[SDL_SCANCODE_C] = KeyCode::C;
    keymap[SDL_SCANCODE_D] = KeyCode::D;
    keymap[SDL_SCANCODE_E] = KeyCode::E;
    keymap[SDL_SCANCODE_F] = KeyCode::F;
    keymap[SDL_SCANCODE_G] = KeyCode::G;
    keymap[SDL_SCANCODE_H] = KeyCode::H;
    keymap[SDL_SCANCODE_I] = KeyCode::I;
    keymap[SDL_SCANCODE_J] = KeyCode::J;
    keymap[SDL_SCANCODE_K] = KeyCode::K;
    keymap[SDL_SCANCODE_L] = KeyCode::L;
    keymap[SDL_SCANCODE_M] = KeyCode::M;
    keymap[SDL_SCANCODE_N] = KeyCode::N;
    keymap[SDL_SCANCODE_O] = KeyCode::O;
    keymap[SDL_SCANCODE_P] = KeyCode::P;
    keymap[SDL_SCANCODE_Q] = KeyCode::Q;
    keymap[SDL_SCANCODE_R] = KeyCode::R;
    keymap[SDL_SCANCODE_S] = KeyCode::S;
    keymap[SDL_SCANCODE_T] = KeyCode::T;
    keymap[SDL_SCANCODE_U] = KeyCode::U;
    keymap[SDL_SCANCODE_V] = KeyCode::V;
    keymap[SDL_SCANCODE_W] = KeyCode::W;
    keymap[SDL_SCANCODE_X] = KeyCode::X;
    keymap[SDL_SCANCODE_Y] = KeyCode::Y;
    keymap[SDL_SCANCODE_Z] = KeyCode::Z;

    keymap[SDL_SCANCODE_LEFTBRACKET] = KeyCode::LeftBracket;
    keymap[SDL_SCANCODE_BACKSLASH] = KeyCode::Backslash;
    keymap[SDL_SCANCODE_RIGHTBRACKET] = KeyCode::RightBracket;
    keymap[SDL_SCANCODE_GRAVE] = KeyCode::GraveAccent;

    keymap[SDL_SCANCODE_INTERNATIONAL1] = KeyCode::World1;
    keymap[SDL_SCANCODE_INTERNATIONAL2] = KeyCode::World2;

    keymap[SDL_SCANCODE_ESCAPE] = KeyCode::Escape;
    keymap[SDL_SCANCODE_RETURN] = KeyCode::Enter;
    keymap[SDL_SCANCODE_TAB] = KeyCode::Tab;
    keymap[SDL_SCANCODE_BACKSPACE] = KeyCode::Backspace;
    keymap[SDL_SCANCODE_INSERT] = KeyCode::Insert;
    keymap[SDL_SCANCODE_DELETE] = KeyCode::Delete;
    keymap[SDL_SCANCODE_RIGHT] = KeyCode::Right;
    keymap[SDL_SCANCODE_LEFT] = KeyCode::Left;
    keymap[SDL_SCANCODE_DOWN] = KeyCode::Down;
    keymap[SDL_SCANCODE_UP] = KeyCode::Up;
    keymap[SDL_SCANCODE_PAGEUP] = KeyCode::PageUp;
    keymap[SDL_SCANCODE_PAGEDOWN] = KeyCode::PageDown;
    keymap[SDL_SCANCODE_HOME] = KeyCode::Home;
    keymap[SDL_SCANCODE_END] = KeyCode::End;
    keymap[SDL_SCANCODE_CAPSLOCK] = KeyCode::CapsLock;
    keymap[SDL_SCANCODE_SCROLLLOCK] = KeyCode::ScrollLock;
    keymap[SDL_SCANCODE_NUMLOCKCLEAR] = KeyCode::NumLock;
    keymap[SDL_SCANCODE_PRINTSCREEN] = KeyCode::PrintScreen;
    keymap[SDL_SCANCODE_PAUSE] = KeyCode::Pause;
    keymap[SDL_SCANCODE_F1] = KeyCode::F1;
    keymap[SDL_SCANCODE_F2] = KeyCode::F2;
    keymap[SDL_SCANCODE_F3] = KeyCode::F3;
    keymap[SDL_SCANCODE_F4] = KeyCode::F4;
    keymap[SDL_SCANCODE_F5] = KeyCode::F5;
    keymap[SDL_SCANCODE_F6] = KeyCode::F6;
    keymap[SDL_SCANCODE_F7] = KeyCode::F7;
    keymap[SDL_SCANCODE_F8] = KeyCode::F8;
    keymap[SDL_SCANCODE_F9] = KeyCode::F9;
    keymap[SDL_SCANCODE_F10] = KeyCode::F10;
    keymap[SDL_SCANCODE_F11] = KeyCode::F11;
    keymap[SDL_SCANCODE_F12] = KeyCode::F12;
    keymap[SDL_SCANCODE_F13] = KeyCode::F13;
    keymap[SDL_SCANCODE_F14] = KeyCode::F14;
    keymap[SDL_SCANCODE_F15] = KeyCode::F15;
    keymap[SDL_SCANCODE_F16] = KeyCode::F16;
    keymap[SDL_SCANCODE_F17] = KeyCode::F17;
    keymap[SDL_SCANCODE_F18] = KeyCode::F18;
    keymap[SDL_SCANCODE_F19] = KeyCode::F19;
    keymap[SDL_SCANCODE_F20] = KeyCode::F20;
    keymap[SDL_SCANCODE_F21] = KeyCode::F21;
    keymap[SDL_SCANCODE_F22] = KeyCode::F22;
    keymap[SDL_SCANCODE_F23] = KeyCode::F23;
    keymap[SDL_SCANCODE_F24] = KeyCode::F24;

    keymap[SDL_SCANCODE_KP_0] = KeyCode::KP0;
    keymap[SDL_SCANCODE_KP_1] = KeyCode::KP1;
    keymap[SDL_SCANCODE_KP_2] = KeyCode::KP2;
    keymap[SDL_SCANCODE_KP_3] = KeyCode::KP3;
    keymap[SDL_SCANCODE_KP_4] = KeyCode::KP4;
    keymap[SDL_SCANCODE_KP_5] = KeyCode::KP5;
    keymap[SDL_SCANCODE_KP_6] = KeyCode::KP6;
    keymap[SDL_SCANCODE_KP_7] = KeyCode::KP7;
    keymap[SDL_SCANCODE_KP_8] = KeyCode::KP8;
    keymap[SDL_SCANCODE_KP_9] = KeyCode::KP9;
    keymap[SDL_SCANCODE_KP_DECIMAL] = KeyCode::KPDecimal;
    keymap[SDL_SCANCODE_KP_DIVIDE] = KeyCode::KPDivide;
    keymap[SDL_SCANCODE_KP_MULTIPLY] = KeyCode::KPMultiply;
    keymap[SDL_SCANCODE_KP_MINUS] = KeyCode::KPSubtract;
    keymap[SDL_SCANCODE_KP_PLUS] = KeyCode::KPAdd;
    keymap[SDL_SCANCODE_KP_ENTER] = KeyCode::KPEnter;
    keymap[SDL_SCANCODE_KP_EQUALS] = KeyCode::KPEqual;

//    keymap[SDL_SCANCODE_X] = KeyCode::LeftShift;
//    keymap[SDL_SCANCODE_X] = KeyCode::LeftControl;
//    keymap[SDL_SCANCODE_X] = KeyCode::LeftAlt;
//    keymap[SDL_SCANCODE_X] = KeyCode::LeftSuper;
//    keymap[SDL_SCANCODE_X] = KeyCode::RightShift;
//    keymap[SDL_SCANCODE_X] = KeyCode::RightControl;
//    keymap[SDL_SCANCODE_X] = KeyCode::RightAlt;
//    keymap[SDL_SCANCODE_X] = KeyCode::RightSuper;
    keymap[SDL_SCANCODE_MENU] = KeyCode::Menu;
}

void SDLInput::beginFrame() {
    pressedKeys.clear();
    releasedKeys.clear();
}

void SDLInput::keyDown(const KeyPressedEvent &event) {
    auto code = event.getKeyCode();
    pressedKeys[code] = !heldKeys[code];
    heldKeys[code] = true;
}

void SDLInput::keyUp(const KeyReleasedEvent &event) {
    auto code = event.getKeyCode();
    releasedKeys[code] = true;
    heldKeys[code] = false;
}

bool SDLInput::isKeyHeld(KeyCode key) {
    return heldKeys[key];
}

bool SDLInput::wasKeyPressed(KeyCode key) {
    return pressedKeys[key];
}

bool SDLInput::wasKeyReleased(KeyCode key) {
    return releasedKeys[key];
}

void SDLInput::mouseButtonDown(const MouseButtonPressedEvent &event) {
    if (event.getMouseButton() == ButtonLeft) {
        buttonPressed = true;
    }
}

void SDLInput::mouseButtonUp(const MouseButtonReleasedEvent &event) {
    if (event.getMouseButton() == ButtonLeft) {
        buttonPressed = false;
    }
}

bool SDLInput::isButtonPressed() const {
    return buttonPressed;
}

void SDLInput::mouseMove(const MouseMovedEvent &event) {
    moveDelta = { event.getX(), event.getY() };
}

void SDLInput::mouseScroll(const MouseScrolledEvent &event) {
    scrollDelta.x = event.getXOffset();
    scrollDelta.y = event.getYOffset();
}

glm::vec2 SDLInput::getScrollDelta() {
    if (lastScrollX < scrollDelta.x) {
        lastScrollX = scrollDelta.x;
        return scrollDelta;
    }
    return glm::vec2(0.f);
}

KeyCode SDLInput::mapScanCodeToKeyCode(const SDL_Scancode& scancode) {
    return keymap[scancode];
}

MouseCode SDLInput::mapMouseButton(uint8_t index) {
    return index - 1;
}
