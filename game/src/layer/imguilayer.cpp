#include "imguilayer.hpp"
#include "maze.hpp"
#include "resourcemanager.hpp"
#include "version.h"
#include <imgui.h>
#include <algorithm>

constexpr ImColor DARK_DEBUG_COLOR{ .3F, .8F, .8F, 1.F };
constexpr ImColor DARK_ERROR_COLOR{ .7F, .3F, 0.3F, 1.F };
constexpr ImColor DARK_NORMAL_COLOR{ 1.F, 1.F, 1.F, 1.F };
constexpr ImColor DARK_WARN_COLOR{ .8F, .8F, 0.3F, 1.F };
const std::string cameraName{ "maze" };

namespace game::layer {

const std::vector logLevels{
    SPDLOG_LEVEL_NAME_TRACE.data(), SPDLOG_LEVEL_NAME_DEBUG.data(),
    SPDLOG_LEVEL_NAME_INFO.data(),  SPDLOG_LEVEL_NAME_WARNING.data(),
    SPDLOG_LEVEL_NAME_ERROR.data(), SPDLOG_LEVEL_NAME_CRITICAL.data(),
    SPDLOG_LEVEL_NAME_OFF.data()
};

const std::vector categories{ "categories", "app", "sponge", "opengl" };

ImGuiLayer::ImGuiLayer() : Layer("imgui") {
    // nothing
}

void ImGuiLayer::onImGuiRender() {
    const auto& io = ImGui::GetIO();

    const auto width = static_cast<float>(
        sponge::platform::sdl::Application::get().getWindowWidth());
    const auto height = static_cast<float>(
        sponge::platform::sdl::Application::get().getWindowHeight());

    static auto hasVsync =
        sponge::platform::sdl::Application::hasVerticalSync();
    auto isFullscreen =
        sponge::platform::sdl::Application::get().isFullscreen();
    auto isWireframeActive = Maze::get().getMazeLayer()->isWireframeActive();

    ImGui::SetNextWindowPos({ width - 320.F, 0.F });
    ImGui::SetNextWindowSize({ 320.F, 360.F });

    constexpr auto windowFlags =
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("App Info", nullptr, windowFlags)) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s %s", game::project_name.data(),
                    game::project_version.data());
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.F / io.Framerate,
                    io.Framerate);
        ImGui::Separator();

        ImGui::BeginTable(
            "##Table", 2,
            ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX);

        auto camera = ResourceManager::getGameCamera(cameraName);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("FOV");
        ImGui::TableNextColumn();
        ImGui::Text("%.0f", camera->getFov());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Pitch");
        ImGui::TableNextColumn();
        ImGui::Text("%.0f", camera->getPitch());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Yaw");
        ImGui::TableNextColumn();
        ImGui::Text("%.0f", camera->getYaw());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Resolution");
        ImGui::TableNextColumn();
        ImGui::Text(
            "%dx%d", sponge::platform::sdl::Application::get().getWindowWidth(),
            sponge::platform::sdl::Application::get().getWindowHeight());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Vertical Sync");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##vertical-sync", &hasVsync)) {
            sponge::platform::sdl::Application::setVerticalSync(hasVsync);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Full Screen");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##fullscreen", &isFullscreen)) {
            sponge::platform::sdl::Application::get().toggleFullscreen();
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Show Wireframe");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##wireframe", &isWireframeActive)) {
            Maze::get().getMazeLayer()->setWireframeActive(isWireframeActive);
        }

        ImGui::EndTable();
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* const layerStack =
                sponge::platform::sdl::Application::get().getLayerStack();
            showLayersTable(layerStack);
        }

        ImGui::End();
    }

    ImGui::SetNextWindowPos({ 0.F, height - 220.F });
    ImGui::SetNextWindowSize({ width, 220.F });

    if (ImGui::Begin("Logging", nullptr,
                     windowFlags | ImGuiWindowFlags_NoScrollbar)) {
        showLogging();
        ImGui::End();
    }
}

float ImGuiLayer::getLogSelectionMaxWidth(
    const std::vector<const char*>& list) {
    float maxWidth = 0;
    for (const auto* item : list) {
        const auto width = ImGui::CalcTextSize(item).x;
        maxWidth = std::max(width, maxWidth);
    }

    return maxWidth + (ImGui::GetStyle().FramePadding.x * 2) +
           ImGui::GetFrameHeight();
}

void ImGuiLayer::showLayersTable(sponge::layer::LayerStack* const layerStack) {
    const auto activeColor = ImGui::GetColorU32(ImVec4(.3F, .7F, .3F, .35F));
    const auto inactiveColor = ImGui::GetColorU32(ImVec4(.5F, .5F, .3F, .3F));

    if (ImGui::BeginTable("layerTable", 1)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

        for (auto layer = layerStack->rbegin(); layer != layerStack->rend();
             ++layer) {
            ImGui::TableNextRow();

            const ImU32 cellBgColor =
                (*layer)->isActive() ? activeColor : inactiveColor;

            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, cellBgColor);

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", (*layer)->getName().c_str());
        }

        ImGui::PopStyleVar();

        ImGui::EndTable();
    }
}

void ImGuiLayer::showLogging() {
    static ImGuiTextFilter filter;
    static auto logLevelWidth = getLogSelectionMaxWidth(logLevels);
    static auto categoriesWidth = getLogSelectionMaxWidth(categories);
    static spdlog::level::level_enum activeLogLevel = spdlog::get_level();
    static auto activeCategory = 0;

    // C++ does not have a case-insensitive string compare
    auto stricmp = [](const std::string& str1, const std::string& str2) {
        return str1.size() == str2.size() &&
               std::equal(str1.begin(), str1.end(), str2.begin(),
                          [](auto a, auto b) {
                              return std::tolower(a) == std::tolower(b);
                          });
    };

    ImGui::SetNextItemWidth(logLevelWidth);
    ImGui::Combo("##activeLogLevel", reinterpret_cast<int*>(&activeLogLevel),
                 logLevels.data(), std::size(logLevels));
    ImGui::SameLine();

    ImGui::SetNextItemWidth(categoriesWidth);
    ImGui::Combo("##categories", &activeCategory, categories.data(),
                 std::size(categories));
    ImGui::SameLine();

    ImGui::TextUnformatted("Filter:");
    ImGui::SameLine();

    filter.Draw("##filter", ImGui::GetWindowWidth() - ImGui::GetCursorPosX() -
                                ImGui::CalcTextSize("Reset").x -
                                (ImGui::GetStyle().FramePadding.x * 6));
    ImGui::SameLine();

    if (ImGui::Button("Reset")) {
        activeLogLevel = spdlog::get_level();
        activeCategory = 0;
        filter.Clear();
    }

    ImGui::Separator();
    ImGui::BeginChild("LogTextView",
                      ImVec2(0, -ImGui::GetStyle().ItemSpacing.y),
                      ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
    ImVec4 color;

    for (const auto& [message, loggerName, level] :
         sponge::platform::sdl::Application::get().getMessages()) {
        if (level < activeLogLevel) {
            continue;
        }

        const auto category = std::string(categories[activeCategory]);
        if (activeCategory > 0 && !stricmp(loggerName, category)) {
            continue;
        }

        if (!filter.PassFilter(message.c_str())) {
            continue;
        }

        bool hasColor;
        switch (level) {
            case spdlog::level::debug:
                hasColor = true;
                color = DARK_DEBUG_COLOR;
                break;
            case spdlog::level::warn:
                hasColor = true;
                color = DARK_WARN_COLOR;
                break;
            case spdlog::level::err:
                hasColor = true;
                color = DARK_ERROR_COLOR;
                break;
            default:
                hasColor = false;
                break;
        }

        if (hasColor) {
            ImGui::PushStyleColor(ImGuiCol_Text, color);
        }

        ImGui::TextUnformatted(message.c_str());

        if (hasColor) {
            ImGui::PopStyleColor();
        }
    }

    ImGui::PopStyleVar();

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.F);
    }

    ImGui::EndChild();
}

}  // namespace game::layer
