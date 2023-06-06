#pragma once

#include "sponge.h"

static const char* const COFFEE_TEXTURE = "coffee";
static const char* const GOTHIC_FONT = "league-gothic";
static const char* const SPRITE_SHADER = "sprite";
static const char* const TEXT_SHADER = "text";

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
