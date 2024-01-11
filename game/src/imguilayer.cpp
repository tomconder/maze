#include "imguilayer.h"
#include "imgui.h"
#include "version.h"

ImGuiLayer::ImGuiLayer() : Layer("imgui") {
    // nothing
}

void ImGuiLayer::onImGuiRender() {
    const auto& io = ImGui::GetIO();

    bool open = true;

    ImGui::Begin("maze", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoSavedSettings);

    ImGui::AlignTextToFramePadding();

    const auto title = fmt::format("{} {} {}", game::project_name,
                                   game::project_version, game::git_sha);
    ImGui::Text(title.c_str());
    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0F / io.Framerate,
                io.Framerate);

    if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
        showLayersTable();
    }

    ImGui::End();
}

void ImGuiLayer::showLayersTable() {
    const auto activeColor = ImGui::GetColorU32(ImVec4(.3F, .7F, .3F, .35F));
    const auto inactiveColor = ImGui::GetColorU32(ImVec4(.5F, .5F, .3F, .3F));

    auto* const layerStack = sponge::SDLEngine::get().getLayerStack();

    if (ImGui::BeginTable("layerTable", 1)) {
        for (auto layer = layerStack->rbegin(); layer != layerStack->rend();
             ++layer) {
            ImGui::TableNextRow();

            const ImU32 cellBgColor =
                (*layer)->isActive() ? activeColor : inactiveColor;

            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, cellBgColor);

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s %s", (*layer)->isActive() ? "*" : "-",
                        (*layer)->getName().c_str());
        }

        ImGui::EndTable();
    }
}
