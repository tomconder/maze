#pragma once

#include "sponge.h"

class ExitLayer : public sponge::Layer {
   public:
    void onAttach() override;
    void onDetach() override;
    void onEvent(sponge::Event& event) override;
    bool onUpdate(uint32_t elapsedTime) override;

    void setWidthAndHeight(uint32_t width, uint32_t height);

   private:
    std::shared_ptr<sponge::OpenGLFont> font;
    std::unique_ptr<sponge::OrthoCamera> orthoCamera;
    std::unique_ptr<sponge::OpenGLQuad> quad;

    bool onKeyPressed(const sponge::KeyPressedEvent& event);
    bool onMouseMoved(const sponge::MouseMovedEvent& event);
    bool onMouseScrolled(const sponge::MouseScrolledEvent& event);
    bool onWindowResize(const sponge::WindowResizeEvent& event);
};