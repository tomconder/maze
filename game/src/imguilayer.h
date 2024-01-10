#pragma once

#include "sponge.h"

class ImGuiLayer : public sponge::graphics::layer::Layer {
   public:
    ImGuiLayer();
    void onImGuiRender() override;

   private:
    static void showLayers();
};
