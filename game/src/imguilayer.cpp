#include "imguilayer.h"
#include "imgui.h"
#include "version.h"

void ImGuiLayer::onImGuiRender() {
    ImGuiIO& io = ImGui::GetIO();

    const auto title =
        fmt::format("Maze {} {}", game::project_version, game::git_sha);
    ImGui::Begin(title.c_str());

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0F / io.Framerate, io.Framerate);

    ImGui::End();
}

void ImGuiLayer::onAttach() {
    Layer::onAttach();
}
