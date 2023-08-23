#pragma once

#include "sponge.h"

class HUDLayer : public sponge::Layer {
   public:
    void onAttach() override;
    void onDetach() override;
    void onEvent(sponge::Event& event) override;
    bool onUpdate(uint32_t elapsedTime) override;

   private:
    std::shared_ptr<sponge::OpenGLFont> font;
    std::shared_ptr<sponge::OpenGLSprite> logo;
    std::unique_ptr<sponge::OrthoCamera> orthoCamera;

    bool onWindowResize(const sponge::WindowResizeEvent& event);
};
