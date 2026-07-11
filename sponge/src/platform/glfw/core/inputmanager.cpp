#include "platform/glfw/core/inputmanager.hpp"

#include <cmath>
#include <cstring>

#include "input/gameaction.hpp"
#include "input/inputcontext.hpp"
#include "input/keycode.hpp"
#include "input/mousecode.hpp"
#include "logging/log.hpp"

namespace sponge::platform::glfw::core {

InputManager* InputManager::instance = nullptr;

static_assert(+input::InputContext::Menu + 1 == 2,
              "Update bindingMaps array size to match InputContext values");

void InputManager::onAttach(GLFWwindow* window) {
    this->window = window;

    // Scan for already-connected gamepad
    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; jid++) {
        if (glfwJoystickIsGamepad(jid)) {
            snapshot.gamepadSlot      = jid;
            snapshot.gamepadConnected = true;
            break;
        }
    }

    // Seed cursor position so first frame has zero delta
    glfwGetCursorPos(window, &prevCursorX, &prevCursorY);

    // Install joystick hot-plug callback
    instance = this;
    glfwSetJoystickCallback(joystickCallback);

    buildDefaultBindings();
}

void InputManager::onDetach() {
    glfwSetJoystickCallback(nullptr);
    instance = nullptr;
    window   = nullptr;
}

void InputManager::pushContext(const input::InputContext ctx) {
    setActiveContext(ctx);
}

void InputManager::popContext() {
    // No-op: context is set per-frame via setActiveContext(); push/pop
    // is no longer used but kept for API compatibility.
}

void InputManager::setActiveContext(const input::InputContext ctx) {
    // Menu always wins: if any layer requests Menu this frame, Menu is used
    // for the next resolveActions() call regardless of write order between
    // concurrent update and render threads. Gameplay writes are skipped
    // because Gameplay (0) is the value we reset to at the start of each frame.
    if (ctx == input::InputContext::Menu) {
        pendingContext.store(1, std::memory_order_release);
    }
}

input::InputContext InputManager::activeContext() const {
    return resolvedContext;
}

void InputManager::onMouseWarped() {
    glfwGetCursorPos(window, &prevCursorX, &prevCursorY);
    cursorDeltaX = 0.0;
    cursorDeltaY = 0.0;
}

void InputManager::recenterCursor() {
    if (!mouseLookActive.load(std::memory_order_acquire)) {
        return;
    }
    int width  = 0;
    int height = 0;
    glfwGetWindowSize(window, &width, &height);
    const double centerX = width / 2.0;
    const double centerY = height / 2.0;
    glfwSetCursorPos(window, centerX, centerY);
    prevCursorX = centerX;
    prevCursorY = centerY;
}

void InputManager::joystickCallback(const int jid, const int event) {
    if (!instance) {
        return;
    }
    if (event == GLFW_CONNECTED) {
        if (glfwJoystickIsGamepad(jid) && instance->snapshot.gamepadSlot < 0) {
            instance->snapshot.gamepadSlot      = jid;
            instance->snapshot.gamepadConnected = true;
        }
    } else if (event == GLFW_DISCONNECTED) {
        if (jid == instance->snapshot.gamepadSlot) {
            instance->snapshot.gamepadConnected = false;
            instance->snapshot.gamepadSlot      = -1;
            std::memset(&instance->gamepadStateCurrent, 0,
                        sizeof(instance->gamepadStateCurrent));
        }
    }
}

void InputManager::update() {
    // 1. Copy previous state for edge detection
    keyPrev   = keyDown;
    mousePrev = mouseDown;

    // 2. Poll keyboard and mouse button state
    for (int key = 0; key <= GLFW_KEY_LAST; key++) {
        keyDown[key] = glfwGetKey(window, key) == GLFW_PRESS;
    }
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; button++) {
        mouseDown[button] = glfwGetMouseButton(window, button) == GLFW_PRESS;
    }

    // 3. Poll cursor delta (main thread only)
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window, &cursorX, &cursorY);
    cursorDeltaX = cursorX - prevCursorX;
    cursorDeltaY = cursorY - prevCursorY;

    recenterCursor();
    if (!mouseLookActive.load(std::memory_order_acquire)) {
        prevCursorX = cursorX;
        prevCursorY = cursorY;
    }

    // 4. Poll gamepad
    pollGamepad();

    // 5. Determine which device was used this frame
    updateActiveDevice();

    // 6. Latch context: read what layers set last frame, reset to Gameplay (0)
    //    for the next frame. Menu (1) overrides Gameplay due to
    //    setActiveContext.
    resolvedContext = static_cast<input::InputContext>(
        pendingContext.exchange(0, std::memory_order_acq_rel));

    // 7. Map bindings to snapshot
    resolveActions();
}

void InputManager::pollGamepad() {
    gamepadStatePrev = gamepadStateCurrent;

    if (snapshot.gamepadSlot >= 0) {
        if (!glfwGetGamepadState(snapshot.gamepadSlot, &gamepadStateCurrent)) {
            // Slot became invalid — treat as disconnect
            snapshot.gamepadConnected = false;
            snapshot.gamepadSlot      = -1;
            std::memset(&gamepadStateCurrent, 0, sizeof(gamepadStateCurrent));
        }
    }
}

void InputManager::updateActiveDevice() {
    // Gamepad is checked first so that driver-level button aliases (e.g.
    // Start → Escape, D-pad → arrow keys) cannot force-switch the active device
    // while a physical gamepad button or axis is held.
    if (snapshot.gamepadConnected) {
        for (int axis = 0; axis <= GLFW_GAMEPAD_AXIS_LAST; axis++) {
            const float axisValue = gamepadStateCurrent.axes[axis];
            // Trigger axes rest at -1.0 (released) and travel to +1.0
            // (pressed). Use a midpoint threshold rather than abs() so the
            // resting position does not falsely register as active input.
            const bool isTrigger = (axis == GLFW_GAMEPAD_AXIS_LEFT_TRIGGER ||
                                    axis == GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER);
            const bool axisActive =
                isTrigger ? (axisValue > -0.5f) : (std::abs(axisValue) > 0.1f);
            if (axisActive) {
                snapshot.activeDevice = input::ActiveDevice::Gamepad;
                return;
            }
        }
        for (int button = 0; button <= GLFW_GAMEPAD_BUTTON_LAST; button++) {
            if (gamepadStateCurrent.buttons[button] == GLFW_PRESS) {
                snapshot.activeDevice = input::ActiveDevice::Gamepad;
                return;
            }
        }
    }

    // Physical Escape switches to KeyboardMouse. Placed after the gamepad
    // check so a driver-aliased Escape (e.g. from Start) does not falsely
    // set the device while the gamepad button is held.
    if (keyDown[+input::KeyCode::SpongeKey_Escape]) {
        snapshot.activeDevice = input::ActiveDevice::KeyboardMouse;
        return;
    }

    // Arrow keys switch to KeyboardMouse. No early return — the gamepad check
    // above has already run, so there is nothing left to override this.
    if (keyDown[+input::KeyCode::SpongeKey_Up] ||
        keyDown[+input::KeyCode::SpongeKey_Down] ||
        keyDown[+input::KeyCode::SpongeKey_Left] ||
        keyDown[+input::KeyCode::SpongeKey_Right]) {
        snapshot.activeDevice = input::ActiveDevice::KeyboardMouse;
    }

    for (int key = 0; key <= GLFW_KEY_LAST; key++) {
        if (keyDown[key]) {
            snapshot.activeDevice = input::ActiveDevice::KeyboardMouse;
            return;
        }
    }
    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; button++) {
        if (mouseDown[button]) {
            snapshot.activeDevice = input::ActiveDevice::KeyboardMouse;
            return;
        }
    }
    if (std::abs(cursorDeltaX) > 0.5 || std::abs(cursorDeltaY) > 0.5) {
        snapshot.activeDevice = input::ActiveDevice::KeyboardMouse;
    }
}

void InputManager::resolveActions() {
    using input::BindingType;
    using input::GameAction;

    const auto& bindingMap = bindingMaps[static_cast<uint8_t>(activeContext())];
    snapshot.active.fill(false);
    snapshot.held.fill(false);
    snapshot.axis.fill(0.f);

    for (int action = 0; action < +GameAction::Count; action++) {
        for (const auto& [type, rawCode, axisScale, deadzone] :
             bindingMap[action]) {
            bool  newHeld   = false;
            bool  newActive = false;
            float newAxis   = 0.f;

            const bool gamepadActive =
                snapshot.activeDevice == input::ActiveDevice::Gamepad;

            switch (type) {
                case BindingType::Key: {
                    // Suppress keyboard bindings when gamepad is active to
                    // prevent D-pad ghost-firing arrow key events (driver-level
                    // aliasing).
                    if (gamepadActive)
                        break;
                    const int key = rawCode;
                    if (key > 0 && key <= GLFW_KEY_LAST) {
                        newHeld   = keyDown[key];
                        newActive = keyDown[key] && !keyPrev[key];
                        newAxis   = newHeld ? axisScale : 0.f;
                    }
                    break;
                }
                case BindingType::MouseButton: {
                    if (gamepadActive)
                        break;
                    const int mouseButton = rawCode;
                    if (mouseButton >= 0 &&
                        mouseButton <= GLFW_MOUSE_BUTTON_LAST) {
                        newHeld = mouseDown[mouseButton];
                        newActive =
                            mouseDown[mouseButton] && !mousePrev[mouseButton];
                        newAxis = newHeld ? axisScale : 0.f;
                    }
                    break;
                }
                case BindingType::MouseAxisX: {
                    if (gamepadActive)
                        break;
                    newAxis   = static_cast<float>(cursorDeltaX) * axisScale;
                    newHeld   = std::abs(newAxis) > deadzone;
                    newActive = newHeld;
                    break;
                }
                case BindingType::MouseAxisY: {
                    if (gamepadActive)
                        break;
                    newAxis   = static_cast<float>(cursorDeltaY) * axisScale;
                    newHeld   = std::abs(newAxis) > deadzone;
                    newActive = newHeld;
                    break;
                }
                case BindingType::GamepadButton: {
                    if (snapshot.gamepadConnected) {
                        const int gamepadButton = rawCode;
                        if (gamepadButton >= 0 &&
                            gamepadButton <= GLFW_GAMEPAD_BUTTON_LAST) {
                            newHeld =
                                gamepadStateCurrent.buttons[gamepadButton] ==
                                GLFW_PRESS;
                            newActive =
                                newHeld &&
                                gamepadStatePrev.buttons[gamepadButton] !=
                                    GLFW_PRESS;
                            newAxis = newHeld ? axisScale : 0.f;
                        }
                    }
                    break;
                }
                case BindingType::GamepadAxis: {
                    if (snapshot.gamepadConnected) {
                        const int gamepadAxis = rawCode;
                        if (gamepadAxis >= 0 &&
                            gamepadAxis <= GLFW_GAMEPAD_AXIS_LAST) {
                            float raw = gamepadStateCurrent.axes[gamepadAxis] *
                                        axisScale;
                            if (std::abs(raw) < deadzone) {
                                raw = 0.f;
                            }
                            float rawPrev =
                                gamepadStatePrev.axes[gamepadAxis] * axisScale;
                            if (std::abs(rawPrev) < deadzone) {
                                rawPrev = 0.f;
                            }
                            newAxis   = raw;
                            newHeld   = newAxis > 0.f;
                            newActive = newHeld && rawPrev <= 0.f;
                        }
                    }
                    break;
                }
            }

            snapshot.held[action]   = snapshot.held[action] || newHeld;
            snapshot.active[action] = snapshot.active[action] || newActive;
            // Highest-magnitude binding wins
            if (std::abs(newAxis) > std::abs(snapshot.axis[action])) {
                snapshot.axis[action] = newAxis;
            }
        }
    }
}

void InputManager::buildDefaultBindings() {
    using input::BindingType;
    using input::GameAction;
    using input::InputBinding;
    using input::InputContext;
    using input::KeyCode;
    using input::MouseButton;

    auto& menu     = bindingMaps[+InputContext::Menu];
    auto& gameplay = bindingMaps[+InputContext::Gameplay];

    constexpr float keyboardSpeed   = 0.075f;  // keyboard movement speed
    constexpr float mouseSpeed      = 0.125f;  // mouse look sensitivity
    constexpr float gamepadLook     = 0.5f;    // gamepad look sensitivity
    constexpr float gamepadDeadZone = 0.2f;    // gamepad axis deadzone

    // ── Menu ──────────────────────────────────────────────────────────────
    menu[+GameAction::MenuUp] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Up,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_KP8,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::GamepadButton,
                      .rawCode   = GLFW_GAMEPAD_BUTTON_DPAD_UP,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_LEFT_Y,
                      .axisScale = -1.f,
                      .deadzone  = gamepadDeadZone },
    };
    menu[+GameAction::MenuDown] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Down,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_KP2,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::GamepadButton,
                      .rawCode   = GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_LEFT_Y,
                      .axisScale = 1.f,
                      .deadzone  = gamepadDeadZone },
    };
    menu[+GameAction::MenuLeft] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Left,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_KP4,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::GamepadButton,
                      .rawCode   = GLFW_GAMEPAD_BUTTON_DPAD_LEFT,
                      .axisScale = 1.f },
    };
    menu[+GameAction::MenuRight] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Right,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_KP6,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::GamepadButton,
                      .rawCode   = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
                      .axisScale = 1.f },
    };
    menu[+GameAction::MenuConfirm] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Enter,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_KPEnter,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Space,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::GamepadButton,
                      .rawCode   = GLFW_GAMEPAD_BUTTON_A,
                      .axisScale = 1.f },
    };
    menu[+GameAction::MenuBack] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Escape,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::GamepadButton,
                      .rawCode   = GLFW_GAMEPAD_BUTTON_B,
                      .axisScale = 1.f },
    };

    // ── Gameplay ──────────────────────────────────────────────────────────
    gameplay[+GameAction::MoveForward] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_W,
                      .axisScale = keyboardSpeed },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Up,
                      .axisScale = keyboardSpeed },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_LEFT_Y,
                      .axisScale = -keyboardSpeed,
                      .deadzone  = gamepadDeadZone },
    };
    gameplay[+GameAction::MoveBack] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_S,
                      .axisScale = keyboardSpeed },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Down,
                      .axisScale = keyboardSpeed },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_LEFT_Y,
                      .axisScale = keyboardSpeed,
                      .deadzone  = gamepadDeadZone },
    };
    gameplay[+GameAction::MoveLeft] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_A,
                      .axisScale = keyboardSpeed },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Left,
                      .axisScale = keyboardSpeed },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_LEFT_X,
                      .axisScale = -keyboardSpeed,
                      .deadzone  = gamepadDeadZone },
    };
    gameplay[+GameAction::MoveRight] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_D,
                      .axisScale = keyboardSpeed },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Right,
                      .axisScale = keyboardSpeed },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_LEFT_X,
                      .axisScale = keyboardSpeed,
                      .deadzone  = gamepadDeadZone },
    };
    gameplay[+GameAction::LookHorizontal] = {
        InputBinding{ .type      = BindingType::MouseAxisX,
                      .axisScale = mouseSpeed },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_RIGHT_X,
                      .axisScale = gamepadLook,
                      .deadzone  = gamepadDeadZone },
    };
    gameplay[+GameAction::LookVertical] = {
        InputBinding{ .type      = BindingType::MouseAxisY,
                      .axisScale = mouseSpeed },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_RIGHT_Y,
                      .axisScale = gamepadLook,
                      .deadzone  = gamepadDeadZone },
    };
    gameplay[+GameAction::Pause] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Escape,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::GamepadButton,
                      .rawCode   = GLFW_GAMEPAD_BUTTON_START,
                      .axisScale = 1.f },
        InputBinding{ .type      = BindingType::GamepadButton,
                      .rawCode   = GLFW_GAMEPAD_BUTTON_BACK,
                      .axisScale = 1.f },
    };

    // ── Toggle actions (both contexts) ───────────────────────────────────────
    menu[+GameAction::ToggleFullscreen] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_F,
                      .axisScale = 1.f },
    };
    menu[+GameAction::ToggleDebugUI] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_GraveAccent,
                      .axisScale = 1.f },
    };
    gameplay[+GameAction::ToggleFullscreen] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_F,
                      .axisScale = 1.f },
    };
    gameplay[+GameAction::ToggleDebugUI] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_GraveAccent,
                      .axisScale = 1.f },
    };
}

}  // namespace sponge::platform::glfw::core
