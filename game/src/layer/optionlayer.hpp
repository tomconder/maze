#pragma once

#include "sponge.hpp"

namespace game::layer {

enum class OptionMenuItem : uint8_t { Return = 0, Count };

class OptionLayer final : public sponge::layer::Layer {
public:
    OptionLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

private:
    OptionMenuItem selectedItem = OptionMenuItem::Return;

    void recalculateLayout(float width, float height) const;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event) const;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;

    void clearHoveredItems() const;
};

}  // namespace game::layer
