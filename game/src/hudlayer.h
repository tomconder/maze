#pragma once

#include "sponge.h"

class HUDLayer : public sponge::Layer {
   public:
    HUDLayer();
    void onAttach() override;
    void onDetach() override;
    void onEvent(sponge::Event& event) override;
    bool onUpdate(uint32_t elapsedTime) override;

   private:
    std::unique_ptr<sponge::OpenGLSprite> logo;

    bool onWindowResize(const sponge::WindowResizeEvent& event);
};
