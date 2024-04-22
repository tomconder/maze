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
    std::shared_ptr<sponge::renderer::OrthoCamera> orthoCamera;
    std::unique_ptr<sponge::platform::opengl::Sprite> logo;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;
};
