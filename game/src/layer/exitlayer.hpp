#pragma once

#include "sponge.hpp"
#include "ui/button.hpp"

namespace game::layer {

class ExitLayer final : public sponge::layer::Layer {
   public:
    ExitLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

   private:
    std::shared_ptr<sponge::scene::OrthoCamera> orthoCamera;

    const std::string cancelButtonMessage = "Cancel";
    const std::string confirmButtonMessage = "Confirm";
    const std::string message = "Exit the Game?";

    bool isRunning = true;

    std::unique_ptr<sponge::platform::opengl::scene::Quad> quad;
    std::unique_ptr<ui::Button> cancelButton;
    std::unique_ptr<ui::Button> confirmButton;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event) const;

    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);

    bool onMouseMoved(const sponge::event::MouseMovedEvent& event) const;

    static bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event);

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;
};

}  // namespace game::layer
