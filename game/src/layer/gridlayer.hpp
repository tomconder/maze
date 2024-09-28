#pragma once

#include "scene/gamecamera.hpp"
#include "sponge.hpp"

namespace game::layer {

class GridLayer final : public sponge::layer::Layer {
   public:
    GridLayer();

    void onAttach() override;

    void onDetach() override;

    bool onUpdate(double elapsedTime) override;

   private:
    std::string shaderName;

    std::shared_ptr<scene::GameCamera> camera;
    std::unique_ptr<sponge::platform::opengl::scene::Grid> grid;
};

}  // namespace game::layer
