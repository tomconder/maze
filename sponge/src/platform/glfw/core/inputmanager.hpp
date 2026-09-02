// sponge/src/platform/glfw/core/inputmanager.hpp
#pragma once
#include "input/gameaction.hpp"
#include "input/inputbinding.hpp"
#include "input/inputcontext.hpp"
#include "input/inputsnapshot.hpp"

#include <GLFW/glfw3.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <string>

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

    void setActiveContext(input::InputContext ctx);

    // Primary keyboard binding for an action, or 0 when it has none.
    int getPrimaryKey(input::GameAction action) const;

    // Display name for a raw key code, e.g. "W", "Escape", "Unbound".
    static std::string keyLabel(int rawCode);

    // The next key pressed replaces the primary keyboard binding of action in
    // every context that binds it, and is swallowed so it does not also fire
    // the action; Escape cancels. Applied on the main thread in update().
    void requestRebind(const input::GameAction action) {
        rebindAction.store(+action, std::memory_order_release);
    }

    bool isRebinding() const {
        return rebindAction.load(std::memory_order_acquire) >= 0;
    }

    // Drops every stored key override; applied on the main thread in update().
    void requestResetBindings() {
        pendingResetBindings.store(true, std::memory_order_release);
    }

    // Enable continuous cursor recentering while mouse-look is active.
    // Prevents the cursor from drifting outside the window and sending
    // negative coordinates to ImGui.
    void setMouseLookActive(const bool active) {
        mouseLookActive.store(active, std::memory_order_release);
    }

private:
    input::InputContext activeContext() const;

    GLFWwindow* window = nullptr;

    // Written from the update thread (MazeLayer), read from the main thread
    // (recenterCursor/update).
    std::atomic<bool> mouseLookActive{ false };

    input::InputSnapshot snapshot;

    // Context latch: Menu always overrides Gameplay even when set from
    // concurrent threads. Gameplay (0) is the reset value each frame;
    // any layer calling setActiveContext(Menu) stores 1 — last Menu write
    // wins and a Gameplay write never clears a pending Menu.
    std::atomic<uint8_t> pendingContext{ 0 };
    input::InputContext  resolvedContext{ input::InputContext::Gameplay };

    // GameAction awaiting a new key, or -1 when idle. Written by menu layers
    // on the render thread, consumed on the main thread.
    std::atomic<int>  rebindAction{ -1 };
    std::atomic<bool> pendingResetBindings{ false };

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
    void applyBindingOverrides();
    void setPrimaryKey(input::GameAction action, int rawCode);

    // True while a rebind is pending, in which case no actions resolve.
    bool captureRebind();
    void resetBindings();

    void pollGamepad();
    void resolveActions();
    void updateActiveDevice();

    // Static joystick callback for hot-plug; uses instance pointer
    static InputManager* instance;
    static void          joystickCallback(int jid, int event);
};

}  // namespace sponge::platform::glfw::core
