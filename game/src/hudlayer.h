#pragma once

#include "sponge.h"

class HUDLayer final : public sponge::layer::Layer {
   public:
    HUDLayer();
    void onAttach() override;
    void onDetach() override;
    void onEvent(sponge::event::Event& event) override;
    bool onUpdate(double elapsedTime, bool isEventHandled) override;

   private:
    std::unique_ptr<sponge::graphics::renderer::OpenGLSprite> logo;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event);
};
