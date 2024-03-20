#pragma once

#include "sponge.hpp"

class HUDLayer final : public sponge::layer::Layer {
   public:
    HUDLayer();
    void onAttach() override;
    void onDetach() override;
    void onEvent(sponge::event::Event& event) override;
    bool onUpdate(double elapsedTime) override;

   private:
    std::unique_ptr<sponge::renderer::OpenGLSprite> logo;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event);
};
