#pragma once

#include "ui/button.hpp"

namespace game::layer {

enum class ExitMenuItem : uint8_t {
    Continue = 0,
    Options,
    ReturnToMenu,
    Exit,
    Count
};

class ExitLayer final : public sponge::layer::Layer {
public:
    ExitLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

private:
    bool         optionsFlag  = false;
    ExitMenuItem selectedItem = ExitMenuItem::Continue;

    void recalculateLayout(float width, float height) const;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event) const;

    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;

    void clearHoveredItems() const;
};
}  // namespace game::layer
