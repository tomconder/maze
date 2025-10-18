#include "platform/glfw/core/input.hpp"

#include "platform/glfw/core/application.hpp"

#include <tuple>
#include <utility>

namespace sponge::platform::glfw::core {

bool Input::isKeyPressed(const input::KeyCode keycode) {
    return keyData.contains(keycode) &&
           keyData[keycode].state == KeyState::Pressed;
}

bool Input::isKeyHeld(const input::KeyCode keycode) {
    return keyData.contains(keycode) &&
           keyData[keycode].state == KeyState::Held;
}

bool Input::isKeyDown(const input::KeyCode keycode) {
    if (Application::get().isEventHandledByImGui()) {
        return false;
    }

    const auto window = Application::get().window;
    const auto state =
        glfwGetKey(static_cast<GLFWwindow*>(window->getNativeWindow()),
                   static_cast<int32_t>(keycode));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::isKeyReleased(const input::KeyCode keycode) {
    return keyData.contains(keycode) &&
           keyData[keycode].state == KeyState::Released;
}

void Input::updateKeyState(const input::KeyCode keycode, const KeyState state) {
    auto& [key, keyState, oldState] = keyData[keycode];
    key = keycode;
    oldState = keyState;
    keyState = state;
}

bool Input::isMouseButtonPressed(const input::MouseButton button) {
    return mouseData.contains(button) &&
           mouseData[button].state == KeyState::Pressed;
}

bool Input::isMouseButtonHeld(const input::MouseButton button) {
    return mouseData.contains(button) &&
           mouseData[button].state == KeyState::Held;
}

bool Input::isMouseButtonDown(const input::MouseButton button) {
    if (Application::get().isEventHandledByImGui()) {
        return false;
    }

    const auto window = Application::get().window;
    const auto state = glfwGetMouseButton(
        static_cast<GLFWwindow*>(window->getNativeWindow()), button);
    return state == GLFW_PRESS;
}

bool Input::isMouseButtonReleased(const input::MouseButton button) {
    return mouseData.contains(button) &&
           mouseData[button].state == KeyState::Released;
}

float Input::getMouseX() {
    auto [x, y] = getMousePosition();
    return x;
}

float Input::getMouseY() {
    auto [x, y] = getMousePosition();
    return y;
}

std::pair<float, float> Input::getMousePosition() {
    const auto window = Application::get().window;

    double x;
    double y;
    glfwGetCursorPos(static_cast<GLFWwindow*>(window->getNativeWindow()), &x,
                     &y);
    return { static_cast<float>(x), static_cast<float>(y) };
}

std::pair<float, float> Input::getRelativeMousePosition() {
    const auto window = Application::get().window;

    double x;
    double y;
    glfwGetCursorPos(static_cast<GLFWwindow*>(window->getNativeWindow()), &x,
                     &y);
    return { static_cast<float>(x - prevCursorPosX),
             static_cast<float>(y - prevCursorPosY) };
}

void Input::updateButtonState(const input::MouseButton button,
                              const KeyState state) {
    auto& [mouseButton, keyState, oldState] = mouseData[button];
    mouseButton = button;
    oldState = keyState;
    keyState = state;
}

std::pair<double, double> Input::getPrevCursorPos() {
    return { prevCursorPosX, prevCursorPosY };
}

std::pair<double, double> Input::getRelativeCursorPos() {
    return { relativeX, relativeY };
}

bool Input::getCursorEnteredWindow() {
    return cursorEnteredWindow;
}

void Input::setPrevCursorPos(const std::pair<double, double>& val) {
    std::tie(prevCursorPosX, prevCursorPosY) = val;
}

void Input::setRelativeCursorPos(const std::pair<double, double>& val) {
    relativeX = static_cast<float>(val.first);
    relativeY = static_cast<float>(val.second);
}

void Input::setCursorEnteredWindow(const bool val) {
    cursorEnteredWindow = val;
}

}  // namespace sponge::platform::glfw::core
