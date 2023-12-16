#pragma once

#include "sponge.h"

class HUDLayer : public sponge::graphics::layer::Layer {
   public:
    HUDLayer();
    void onAttach() override;
    void onDetach() override;
    void onEvent(sponge::event::Event& event) override;
    bool onUpdate(uint32_t elapsedTime) override;

   private:
    std::unique_ptr<sponge::graphics::renderer::OpenGLSprite> logo;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event);
};
