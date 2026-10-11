#pragma once

#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/mouseevent.hpp"
#include "input/gameaction.hpp"
#include "layer/layer.hpp"

#include <cstdint>
#include <optional>
#include <tuple>

namespace game::layer {

enum class KeyMapItem : uint8_t {
    MoveForward = 0,
    MoveBack,
    MoveLeft,
    MoveRight,
    Pause,
    ToggleFullscreen,
    ToggleDebugUI,
    ResetDefaults,
    Return,
    Count,
};

class KeyMapLayer final : public sponge::layer::Layer {
public:
    KeyMapLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

private:
    KeyMapItem                selectedItem = KeyMapItem::MoveForward;
    std::optional<KeyMapItem> hoveredItem;

    // Row waiting for a key, while InputManager captures the next press.
    std::optional<KeyMapItem> rebindingItem;

    bool wasActiveLastFrame    = false;
    bool waitForConfirmRelease = false;

    void renderRowBackground(float x, float y, float w, float h,
                             KeyMapItem item) const;

    // Screen rect of a row, resolved through the root/menu/background chain.
    static std::tuple<float, float, float, float> rowLayout(KeyMapItem item);

    // Starts a rebind, resets the defaults, or leaves — whichever the row is.
    void activate(KeyMapItem item);

    void close();

    static void recalculateLayout(float width, float height);

    // Lays out again at the current window size, after the scroll moved.
    static void relayout();

    // Scrolls the selected row into view.
    void followSelection();

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event);

    bool onMouseButtonReleased(
        const sponge::event::MouseButtonReleasedEvent& event);

    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event);

    void clearHoveredItems();
};

}  // namespace game::layer
