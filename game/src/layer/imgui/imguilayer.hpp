#pragma once

#include "sponge.hpp"
#include <span>

namespace game::layer::imgui {
class ImGuiLayer final : public sponge::layer::Layer {
   public:
    ImGuiLayer();

    void onImGuiRender() override;

    static float getLogSelectionMaxWidth(std::span<const char* const> list
        );

   private:
    static bool hasAppInfoMenu;
    static bool hasLogMenu;
    static bool hasVsync;
    static bool isFullscreen;

    static void showMenu();

    static void showFontsTable();
    static void showLayersTable(sponge::layer::LayerStack*);
    static void showModelsTable();
    static void showShadersTable();
    static void showTexturesTable();

    static void showLogging();
};
}  // namespace game::layer::imgui
