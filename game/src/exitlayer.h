#pragma once

#include "sponge.h"
#include "ui/button.h"
#include <memory>

class ExitLayer : public sponge::layer::Layer {
   public:
    ExitLayer();
    void onAttach() override;
    void onDetach() override;
    void onEvent(sponge::event::Event& event) override;
    bool onUpdate(double elapsedTime) override;

    void setWidthAndHeight(uint32_t width, uint32_t height) const;

   private:
    const std::string_view cancelButtonMessage = "Cancel";
    const std::string_view confirmButtonMessage = "Confirm";
    const std::string_view message = "Exit the Game?";

    std::unique_ptr<sponge::renderer::OpenGLQuad> quad;
    std::unique_ptr<ui::Button> cancelButton;
    std::unique_ptr<ui::Button> confirmButton;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event) const;
    bool onMouseButtonPressed(
        const sponge::event::MouseButtonPressedEvent& event);
    bool onMouseMoved(const sponge::event::MouseMovedEvent& event) const;
    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event);
    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;

    bool isRunning = true;
};
