// sponge/src/platform/glfw/core/inputmanager.cpp
#include "platform/glfw/core/inputmanager.hpp"

#include "input/gameaction.hpp"
#include "input/inputcontext.hpp"
#include "input/keycode.hpp"
#include "input/mousecode.hpp"
#include "logging/log.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace sponge::platform::glfw::core {

InputManager* InputManager::instance = nullptr;

static_assert(+input::InputContext::Menu + 1 == 2,
              "Update bindingMaps array size to match InputContext values");

void InputManager::onAttach(GLFWwindow* win) {
    window = win;

    // Scan for already-connected gamepad
    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
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
    if (!mouseLookActive) {
        return;
    }
    int w = 0;
    int h = 0;
    glfwGetWindowSize(window, &w, &h);
    const double cx = w / 2.0;
    const double cy = h / 2.0;
    glfwSetCursorPos(window, cx, cy);
    prevCursorX = cx;
    prevCursorY = cy;
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
    for (int k = 0; k <= GLFW_KEY_LAST; ++k) {
        keyDown[k] = glfwGetKey(window, k) == GLFW_PRESS;
    }
    for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b) {
        mouseDown[b] = glfwGetMouseButton(window, b) == GLFW_PRESS;
    }

    // 3. Poll cursor delta (main thread only)
    double cx = 0.0;
    double cy = 0.0;
    glfwGetCursorPos(window, &cx, &cy);
    cursorDeltaX = cx - prevCursorX;
    cursorDeltaY = cy - prevCursorY;

    recenterCursor();
    if (!mouseLookActive) {
        prevCursorX = cx;
        prevCursorY = cy;
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
        for (int i = 0; i <= GLFW_GAMEPAD_AXIS_LAST; ++i) {
            const float val = gamepadStateCurrent.axes[i];
            // Trigger axes rest at -1.0 (released) and travel to +1.0
            // (pressed). Use a midpoint threshold rather than abs() so the
            // resting position does not falsely register as active input.
            const bool isTrigger = (i == GLFW_GAMEPAD_AXIS_LEFT_TRIGGER ||
                                    i == GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER);
            const bool axisActive =
                isTrigger ? (val > -0.5f) : (std::abs(val) > 0.1f);
            if (axisActive) {
                snapshot.activeDevice = input::ActiveDevice::Gamepad;
                return;
            }
        }
        for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; ++i) {
            if (gamepadStateCurrent.buttons[i] == GLFW_PRESS) {
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

    for (int k = 0; k <= GLFW_KEY_LAST; ++k) {
        if (keyDown[k]) {
            snapshot.activeDevice = input::ActiveDevice::KeyboardMouse;
            return;
        }
    }
    for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b) {
        if (mouseDown[b]) {
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

    const auto& bm = bindingMaps[static_cast<uint8_t>(activeContext())];
    snapshot.active.fill(false);
    snapshot.held.fill(false);
    snapshot.axis.fill(0.f);

    for (int i = 0; i < +GameAction::Count; ++i) {
        for (const auto& [type, rawCode, axisScale, deadzone] : bm[i]) {
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
                    const int k = rawCode;
                    if (k > 0 && k <= GLFW_KEY_LAST) {
                        newHeld   = keyDown[k];
                        newActive = keyDown[k] && !keyPrev[k];
                        newAxis   = newHeld ? axisScale : 0.f;
                    }
                    break;
                }
                case BindingType::MouseButton: {
                    if (gamepadActive)
                        break;
                    const int mb = rawCode;
                    if (mb >= 0 && mb <= GLFW_MOUSE_BUTTON_LAST) {
                        newHeld   = mouseDown[mb];
                        newActive = mouseDown[mb] && !mousePrev[mb];
                        newAxis   = newHeld ? axisScale : 0.f;
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
                        const int gb = rawCode;
                        if (gb >= 0 && gb <= GLFW_GAMEPAD_BUTTON_LAST) {
                            newHeld =
                                gamepadStateCurrent.buttons[gb] == GLFW_PRESS;
                            newActive =
                                newHeld &&
                                gamepadStatePrev.buttons[gb] != GLFW_PRESS;
                            newAxis = newHeld ? axisScale : 0.f;
                        }
                    }
                    break;
                }
                case BindingType::GamepadAxis: {
                    if (snapshot.gamepadConnected) {
                        const int ga = rawCode;
                        if (ga >= 0 && ga <= GLFW_GAMEPAD_AXIS_LAST) {
                            float raw =
                                gamepadStateCurrent.axes[ga] * axisScale;
                            if (std::abs(raw) < deadzone) {
                                raw = 0.f;
                            }
                            float rawPrev =
                                gamepadStatePrev.axes[ga] * axisScale;
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

            snapshot.held[i]   = snapshot.held[i] || newHeld;
            snapshot.active[i] = snapshot.active[i] || newActive;
            // Highest-magnitude binding wins
            if (std::abs(newAxis) > std::abs(snapshot.axis[i])) {
                snapshot.axis[i] = newAxis;
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

    constexpr float kSpeed = 0.075f;  // keyboard movement speed
    constexpr float mSpeed = 0.125f;  // mouse look sensitivity
    constexpr float gpLook = 0.5f;    // gamepad look sensitivity
    constexpr float gpDZ   = 0.2f;    // gamepad axis deadzone

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
                      .deadzone  = gpDZ },
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
                      .deadzone  = gpDZ },
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
                      .axisScale = kSpeed },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Up,
                      .axisScale = kSpeed },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_LEFT_Y,
                      .axisScale = -kSpeed,
                      .deadzone  = gpDZ },
    };
    gameplay[+GameAction::MoveBack] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_S,
                      .axisScale = kSpeed },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Down,
                      .axisScale = kSpeed },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_LEFT_Y,
                      .axisScale = kSpeed,
                      .deadzone  = gpDZ },
    };
    gameplay[+GameAction::MoveLeft] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_A,
                      .axisScale = kSpeed },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Left,
                      .axisScale = kSpeed },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_LEFT_X,
                      .axisScale = -kSpeed,
                      .deadzone  = gpDZ },
    };
    gameplay[+GameAction::MoveRight] = {
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_D,
                      .axisScale = kSpeed },
        InputBinding{ .type      = BindingType::Key,
                      .rawCode   = +KeyCode::SpongeKey_Right,
                      .axisScale = kSpeed },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_LEFT_X,
                      .axisScale = kSpeed,
                      .deadzone  = gpDZ },
    };
    gameplay[+GameAction::LookHorizontal] = {
        InputBinding{ .type = BindingType::MouseAxisX, .axisScale = mSpeed },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_RIGHT_X,
                      .axisScale = gpLook,
                      .deadzone  = gpDZ },
    };
    gameplay[+GameAction::LookVertical] = {
        InputBinding{ .type = BindingType::MouseAxisY, .axisScale = mSpeed },
        InputBinding{ .type      = BindingType::GamepadAxis,
                      .rawCode   = GLFW_GAMEPAD_AXIS_RIGHT_Y,
                      .axisScale = gpLook,
                      .deadzone  = gpDZ },
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
