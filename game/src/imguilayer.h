#pragma once

#include "sponge.h"

class ImGuiLayer final : public sponge::layer::Layer {
   public:
    ImGuiLayer();
    void onImGuiRender() override;

   private:
    static void showLayersTable();
};