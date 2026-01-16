#pragma once

#include "scene/orthocamera.hpp"
#include "ui/button.hpp"

#include <yoga/Yoga.h>
#include <memory>

namespace game::layer {
class ExitLayer final : public sponge::layer::Layer {
public:
    ExitLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

private:
    bool isRunning = true;

    void recalculateLayout(float width, float height) const;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event) const;

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event) const;

    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;
};
}  // namespace game::layer
