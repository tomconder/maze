#pragma once

#include "platform/opengl/renderer/grid.hpp"
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
    std::shared_ptr<scene::GameCamera> camera;
    std::unique_ptr<sponge::platform::opengl::renderer::Grid> grid;
};

}  // namespace game::layer
