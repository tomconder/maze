#pragma once

#include "sponge.hpp"

namespace game::layer::imgui {
class ImGuiLayer final : public sponge::layer::Layer {
public:
    ImGuiLayer();

    void onImGuiRender() override;

    static float getLogSelectionMaxWidth(const char* const list[],
                                         std::size_t size);

private:
    static void showLayersTable(sponge::layer::LayerStack*);

    static void showLogging();
};
} // namespace game::layer
