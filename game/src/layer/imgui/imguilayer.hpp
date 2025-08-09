#pragma once

#include "imgui.h"
#include "sponge.hpp"
#include <optional>
#include <span>
#include <utility>

// Forward declarations
struct ImGuiTextFilter;
struct ImVec4;

namespace game::layer::imgui {
class ImGuiLayer final : public sponge::layer::Layer {
public:
    ImGuiLayer();
    void onImGuiRender() override;

private:
    static bool hasAppInfoMenu;
    static bool hasLogMenu;
    static bool hasVsync;
    static bool isFullscreen;

    // Main sections
    static void updateState();
    static void showAppInfoWindow(float width);
    static void showLogWindow(float width, float height);
    static void showMenu();  // App info window sections
    static void showSettingsSection();
    static void showCameraTable();
    static void showAppSettingsTable();
    static void showLightsSection();
    static void showPBRControls();
    static void showAttenuationSlider(int32_t& attenuationIndex);

    static void showShadowMapSection();
    static void showResourcesSection();

    // Resource tables
    static void showFontsTable();
    static void showLayersTable(sponge::layer::LayerStack*);
    static void showModelsTable();
    static void showShadersTable();
    static void showTexturesTable();

    // Logging
    static void showLogging();
    static void showLogControls(ImGuiTextFilter& filter, float logLevelWidth,
                                float categoriesWidth,
                                spdlog::level::level_enum& activeLogLevel,
                                int& activeCategory);
    static void showLogMessages(const ImGuiTextFilter& filter,
                                spdlog::level::level_enum activeLogLevel,
                                int activeCategory);
    static bool shouldShowLogMessage(const std::string& message,
                                     const std::string& loggerName,
                                     spdlog::level::level_enum level,
                                     const ImGuiTextFilter& filter,
                                     spdlog::level::level_enum activeLogLevel,
                                     int activeCategory);
    static void renderLogMessage(const std::string& message,
                                 spdlog::level::level_enum level);
    static std::optional<ImVec4> getLogLevelColor(
        spdlog::level::level_enum level);
    static bool isCaseInsensitiveEqual(const std::string& str1,
                                       const std::string& str2);

    static float getLogSelectionMaxWidth(std::span<const char* const> list);

    template <typename Func>
    static void showResourceTree(const char* name, Func&& func);

    template <typename Func>
    static void showTableRow(Func&& func);

    template <typename Container>
    static void showSimpleTable(const char* tableName,
                                const Container& container);
};

template <typename Func>
void ImGuiLayer::showResourceTree(const char* name, Func&& func) {
    if (ImGui::TreeNode(name)) {
        std::forward<Func>(func)();
        ImGui::TreePop();
    }
}

template <typename Func>
void ImGuiLayer::showTableRow(Func&& func) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    std::forward<Func>(func)();
}

template <typename Container>
void ImGuiLayer::showSimpleTable(const char* tableName,
                                 const Container& container) {
    if (ImGui::BeginTable(tableName, 1)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

        for (const auto& pair : container) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", pair.first.c_str());
        }

        ImGui::PopStyleVar();
        ImGui::EndTable();
    }
}

}  // namespace game::layer::imgui
