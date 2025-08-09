#pragma once
#include "input/keycode.hpp"
#include "input/mousecode.hpp"
#include <map>
#include <utility>

namespace sponge::platform::glfw::core {

enum class KeyState : int8_t { None = -1, Pressed, Held, Released };

struct KeyData {
    input::KeyCode key;
    KeyState state = KeyState::None;
    KeyState oldState = KeyState::None;
};

struct ButtonData {
    input::MouseButton button;
    KeyState state = KeyState::None;
    KeyState oldState = KeyState::None;
};

class Input {
public:
    static bool isKeyPressed(input::KeyCode keycode);
    static bool isKeyHeld(input::KeyCode keycode);
    static bool isKeyDown(input::KeyCode keycode);
    static bool isKeyReleased(input::KeyCode keycode);
    static void updateKeyState(input::KeyCode keycode, KeyState state);

    static bool isMouseButtonPressed(input::MouseButton button);
    static bool isMouseButtonHeld(input::MouseButton button);
    static bool isMouseButtonDown(input::MouseButton button);
    static bool isMouseButtonReleased(input::MouseButton button);
    static float getMouseX();
    static float getMouseY();
    static std::pair<float, float> getMousePosition();
    static std::pair<float, float> getRelativeMousePosition();
    static void updateButtonState(input::MouseButton button, KeyState state);

    static std::pair<double, double> getPrevCursorPos();
    static std::pair<double, double> getRelativeCursorPos();
    static bool getCursorEnteredWindow();
    static void setPrevCursorPos(const std::pair<double, double>& val);
    static void setRelativeCursorPos(const std::pair<double, double>& val);
    static void setCursorEnteredWindow(bool val);

private:
    inline static bool cursorEnteredWindow = false;
    inline static double prevCursorPosX = 0;
    inline static double prevCursorPosY = 0;
    inline static float relativeX = 0;
    inline static float relativeY = 0;

    inline static std::map<input::MouseButton, ButtonData> mouseData;
    inline static std::map<input::KeyCode, KeyData> keyData;
};

}  // namespace sponge::platform::glfw::core
