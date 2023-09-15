#pragma once

#include "sponge.h"
#include "ui/button.h"
#include <memory>

class ExitLayer : public sponge::Layer {
   public:
    ExitLayer();
    void onAttach() override;
    void onDetach() override;
    void onEvent(sponge::Event& event) override;
    bool onUpdate(uint32_t elapsedTime) override;

    void setWidthAndHeight(uint32_t width, uint32_t height);

   private:
    std::unique_ptr<sponge::OpenGLQuad> quad;
    std::unique_ptr<ui::Button> cancelButton;
    std::unique_ptr<ui::Button> confirmButton;

    bool onKeyPressed(const sponge::KeyPressedEvent& event);
    bool onMouseClicked(const sponge::MouseButtonPressedEvent& event);
    bool onMouseMoved(const sponge::MouseMovedEvent& event);
    bool onMouseScrolled(const sponge::MouseScrolledEvent& event);
    bool onWindowResize(const sponge::WindowResizeEvent& event);

    bool isRunning = true;
};
