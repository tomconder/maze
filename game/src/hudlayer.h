#pragma once

#include "sponge.h"

static const char* const COFFEE_TEXTURE = "coffee";
static const char* const GOTHIC_FONT = "league-gothic";
static const char* const SPRITE_SHADER = "sprite";
static const char* const TEXT_SHADER = "text";

class HUDLayer : public Sponge::Layer {
   public:
    void onAttach() override;
    void onDetach() override;
    void onEvent(Sponge::Event& event) override;
    void onResize(uint32_t width, uint32_t height) override;
    void onUpdate(uint32_t elapsedTime) override;

   private:
    std::shared_ptr<Sponge::OpenGLFont> font;
    std::shared_ptr<Sponge::OpenGLSprite> logo;
    std::unique_ptr<Sponge::OrthoCamera> orthoCamera;

    bool onWindowResize(Sponge::WindowResizeEvent& event);
};
