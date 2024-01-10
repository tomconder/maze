#include "imguilayer.h"
#include "imgui.h"
#include "version.h"

ImGuiLayer::ImGuiLayer() : Layer("imgui") {
    // nothing
}

void ImGuiLayer::onImGuiRender() {
    const auto& io = ImGui::GetIO();

    const auto title =
        fmt::format("Maze {} {}", game::project_version, game::git_sha);
    ImGui::Begin(title.c_str());

    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0F / io.Framerate,
                io.Framerate);

    showLayersTable();

    ImGui::End();
}

void ImGuiLayer::showLayersTable() {
    const auto activeColor = ImGui::GetColorU32(ImVec4(.3F, .7F, .3F, .35F));
    const auto inactiveColor = ImGui::GetColorU32(ImVec4(.5F, .5F, .3F, .3F));

    if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto* const stack = sponge::SDLEngine::get().getLayerStack();

        if (ImGui::BeginTable("layerTable", 1)) {
            for (const auto& layer : *stack) {
                ImGui::TableNextRow();

                const ImU32 cellBgColor =
                    layer->isActive() ? activeColor : inactiveColor;

                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, cellBgColor);

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s %s", layer->isActive() ? "*" : "-",
                            layer->getName().c_str());
            }

            ImGui::EndTable();
        }
    }
}
