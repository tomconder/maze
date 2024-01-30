#pragma once

#include "sponge.h"

class ImGuiLayer final : public sponge::layer::Layer {
   public:
    ImGuiLayer();
    void onImGuiRender() override;
    static float getLogSelectionMaxWidth();

   private:
    static void showLayersTable(sponge::layer::LayerStack*);
    static void showLogging();
};
