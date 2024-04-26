#pragma once

#include "sponge.hpp"

class ImGuiLayer final : public sponge::layer::Layer {
   public:
    ImGuiLayer();
    void onImGuiRender() override;
    static float getLogSelectionMaxWidth(const std::vector<const char*>& list);

   private:
    static void showLayersTable(sponge::layer::LayerStack*);
    static void showLogging();
};
