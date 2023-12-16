#pragma once

#include "sponge.h"
#include "ui/button.h"
#include <memory>

class ExitLayer : public sponge::graphics::layer::Layer {
   public:
    ExitLayer();
    void onAttach() override;
    void onDetach() override;
    void onEvent(sponge::event::Event& event) override;
    bool onUpdate(uint32_t elapsedTime) override;

    void setWidthAndHeight(uint32_t width, uint32_t height);

   private:
    const std::string_view cancelButtonMessage = "Cancel";
    const std::string_view confirmButtonMessage = "Confirm";
    const std::string_view message = "Exit the Game?";

    std::unique_ptr<sponge::graphics::renderer::OpenGLQuad> quad;
    std::unique_ptr<ui::Button> cancelButton;
    std::unique_ptr<ui::Button> confirmButton;

    bool onKeyPressed(const sponge::event::KeyPressedEvent& event);
    bool onMouseClicked(const sponge::event::MouseButtonPressedEvent& event);
    bool onMouseMoved(const sponge::event::MouseMovedEvent& event);
    bool onMouseScrolled(const sponge::event::MouseScrolledEvent& event);
    bool onWindowResize(const sponge::event::WindowResizeEvent& event);

    bool isRunning = true;
};
