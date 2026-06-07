// sponge/src/platform/glfw/core/inputmanager.hpp
#pragma once
#include "input/inputbinding.hpp"
#include "input/inputcontext.hpp"
#include "input/inputsnapshot.hpp"

#include <GLFW/glfw3.h>

#include <array>
#include <atomic>
#include <cstring>

namespace sponge::platform::glfw::core {

class InputManager {
public:
    void onAttach(GLFWwindow* window);
    void onDetach();
    void recenterCursor();
    void update();
    void onMouseWarped();

    const input::InputSnapshot& getSnapshot() const {
        return snapshot;
    }

    void consumeActive(const input::GameAction a) {
        snapshot.active[+a] = false;
    }

    void                pushContext(input::InputContext ctx);
    static void         popContext();
    void                setActiveContext(input::InputContext ctx);
    input::InputContext activeContext() const;

    // Enable continuous cursor recentering while mouse-look is active.
    // Prevents the cursor from drifting outside the window and sending
    // negative coordinates to ImGui.
    void setMouseLookActive(const bool active) {
        mouseLookActive = active;
    }

private:
    GLFWwindow* window = nullptr;

    bool mouseLookActive = false;

    input::InputSnapshot snapshot;

    // Context latch: Menu always overrides Gameplay even when set from
    // concurrent threads. Gameplay (0) is the reset value each frame;
    // any layer calling setActiveContext(Menu) stores 1 — last Menu write
    // wins and a Gameplay write never clears a pending Menu.
    std::atomic<uint8_t> pendingContext{ 0 };
    input::InputContext  resolvedContext{ input::InputContext::Gameplay };

    // [0] = Gameplay bindings, [1] = Menu bindings
    std::array<input::BindingMap, 2> bindingMaps;

    // Polled state — main thread only
    std::array<bool, GLFW_KEY_LAST + 1>          keyDown{};
    std::array<bool, GLFW_KEY_LAST + 1>          keyPrev{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> mouseDown{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> mousePrev{};

    double prevCursorX  = 0.0;
    double prevCursorY  = 0.0;
    double cursorDeltaX = 0.0;
    double cursorDeltaY = 0.0;

    GLFWgamepadstate gamepadStateCurrent{};
    GLFWgamepadstate gamepadStatePrev{};

    void buildDefaultBindings();
    void pollGamepad();
    void resolveActions();
    void updateActiveDevice();

    // Static joystick callback for hot-plug; uses instance pointer
    static InputManager* instance;
    static void          joystickCallback(int jid, int event);
};

}  // namespace sponge::platform::glfw::core
