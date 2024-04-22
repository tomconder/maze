#pragma once

#include "gamecamera.hpp"
#include "platform/opengl/grid.hpp"
#include "sponge.hpp"

class GridLayer final : public sponge::layer::Layer {
   public:
    GridLayer();
    void onAttach() override;
    void onDetach() override;
    bool onUpdate(double elapsedTime) override;

   private:
    std::shared_ptr<GameCamera> camera;
    std::unique_ptr<sponge::platform::opengl::Grid> grid;
};
