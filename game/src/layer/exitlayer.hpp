#pragma once

#include "scene/orthocamera.hpp"
#include "ui/button.hpp"

#include <memory>
#include <string>

namespace game::layer {
class ExitLayer final : public sponge::layer::Layer {
public:
    ExitLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

private:
    std::shared_ptr<scene::OrthoCamera> orthoCamera;
    std::string fontShaderName;
    std::string quadShaderName;

    bool isRunning = true;

    std::unique_ptr<sponge::platform::opengl::scene::Quad> quad;
    std::unique_ptr<ui::Button> cancelButton;
    std::unique_ptr<ui::Button> confirmButton;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event) const;

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event) const;

    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;
};
}  // namespace game::layer
