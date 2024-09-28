#pragma once

#include "sponge.hpp"

namespace game::layer {

class HUDLayer final : public sponge::layer::Layer {
   public:
    HUDLayer();

    void onAttach() override;

    void onDetach() override;

    void onEvent(sponge::event::Event& event) override;

    bool onUpdate(double elapsedTime) override;

   private:
    std::shared_ptr<sponge::scene::OrthoCamera> orthoCamera;
    std::unique_ptr<sponge::platform::opengl::scene::Sprite> logo;
    std::string shaderName;

    bool onWindowResize(const sponge::event::WindowResizeEvent& event) const;
};

}  // namespace game::layer
