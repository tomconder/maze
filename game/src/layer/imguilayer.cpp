#include "imguilayer.hpp"
#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/pointlight.hpp"
#include "version.hpp"
#include <imgui.h>
#include <algorithm>

namespace {
constexpr ImColor DARK_DEBUG_COLOR{ .3F, .8F, .8F, 1.F };
constexpr ImColor DARK_ERROR_COLOR{ .7F, .3F, 0.3F, 1.F };
constexpr ImColor DARK_WARN_COLOR{ .8F, .8F, 0.3F, 1.F };
constexpr char cameraName[] = "maze";
constexpr int categoryCount = 4;
constexpr const char* categories[categoryCount] = { "categories", "app",
                                                    "sponge", "opengl" };

constexpr int logLevelCount = 7;
constexpr const char* logLevels[logLevelCount] = {
    SPDLOG_LEVEL_NAME_TRACE.data(), SPDLOG_LEVEL_NAME_DEBUG.data(),
    SPDLOG_LEVEL_NAME_INFO.data(),  SPDLOG_LEVEL_NAME_WARNING.data(),
    SPDLOG_LEVEL_NAME_ERROR.data(), SPDLOG_LEVEL_NAME_CRITICAL.data(),
    SPDLOG_LEVEL_NAME_OFF.data()
};
}  // namespace

namespace game::layer {

using sponge::platform::glfw::core::Application;

ImGuiLayer::ImGuiLayer() : Layer("imgui") {
    // nothing
}

void ImGuiLayer::onImGuiRender() {
    const auto& io = ImGui::GetIO();

    const auto width =
        static_cast<float>(Application::get().window->getWidth());
    const auto height =
        static_cast<float>(Application::get().window->getHeight());

    auto hasVsync = Application::get().hasVerticalSync();
    auto isFullscreen = Application::get().isFullscreen();

    auto ambientOcclusion = Maze::get().getMazeLayer()->getAmbientOcclusion();
    auto ambientStrength = Maze::get().getMazeLayer()->getAmbientStrength();
    auto attenuationIndex = Maze::get().getMazeLayer()->getAttenuationIndex();
    auto metallic = Maze::get().getMazeLayer()->isMetallic();
    auto numLights = Maze::get().getMazeLayer()->getNumLights();
    auto roughness = Maze::get().getMazeLayer()->getRoughness();

    ImGui::SetNextWindowPos({ width - 376.F, 0.F });
    ImGui::SetNextWindowSize({ 376.F, 516.F });

    constexpr auto windowFlags =
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("App Info", nullptr, windowFlags)) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s %s (%s)", project_name.c_str(), project_version.c_str(),
                    git_sha.c_str());
        ImGui::Text("Resolution: %dx%d", Application::get().window->getWidth(),
                    Application::get().window->getHeight());
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.F / io.Framerate,
                    io.Framerate);
        ImGui::Separator();

        ImGui::BeginTable(
            "##CameraTable", 2,
            ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX);

        const auto camera = ResourceManager::getGameCamera(cameraName);

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

        ImGui::EndTable();
        ImGui::Separator();

        ImGui::BeginTable(
            "##AppTable", 2,
            ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Vertical Sync");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##vertical-sync", &hasVsync)) {
            Application::get().setVerticalSync(hasVsync);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Full Screen");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##fullscreen", &isFullscreen)) {
            Application::get().toggleFullscreen();
        }

        ImGui::EndTable();
        ImGui::Separator();

        if (ImGui::SliderInt("Lights", &numLights, 1, 6)) {
            Maze::get().getMazeLayer()->setNumLights(numLights);
        }

        ImGui::Separator();

        if (ImGui::SliderInt("Attenuation", &attenuationIndex, 0, 10)) {
            Maze::get().getMazeLayer()->setAttenuationIndex(attenuationIndex);
        }

        const auto attenuation =
            PointLight::getAttenuationFromIndex(attenuationIndex);
        ImGui::Text("Distance: %3.f [%1.1f, %1.3f, %1.4f]", attenuation.x,
                    attenuation.y, attenuation.z, attenuation.w);

        ImGui::Separator();

        ImGui::Text("PBR:");
        ImGui::BeginTable(
            "PBR", 1,
            ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("Metallic", &metallic)) {
            Maze::get().getMazeLayer()->setMetallic(metallic);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::SliderFloat("Ambient Strength", &ambientStrength, 0.F, 1.F,
                               "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
            Maze::get().getMazeLayer()->setAmbientStrength(ambientStrength);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::SliderFloat("Roughness", &roughness, 0.F, 1.F, "%.3f",
                               ImGuiSliderFlags_AlwaysClamp)) {
            Maze::get().getMazeLayer()->setRoughness(roughness);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::SliderFloat("Ambient Occlusion", &ambientOcclusion, 0.F, 1.F,
                               "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
            Maze::get().getMazeLayer()->setAmbientOcclusion(ambientOcclusion);
        }

        ImGui::EndTable();
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* const layerStack = Application::get().getLayerStack();
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

float ImGuiLayer::getLogSelectionMaxWidth(const char* const list[],
                                          const std::size_t size) {
    float maxWidth = 0;
    for (std::size_t i = 0; i < size; i++) {
        const auto width = ImGui::CalcTextSize(list[i]).x;
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
    static auto logLevelWidth =
        getLogSelectionMaxWidth(logLevels, logLevelCount);
    static auto categoriesWidth =
        getLogSelectionMaxWidth(categories, categoryCount);
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
                 logLevels, logLevelCount);
    ImGui::SameLine();

    ImGui::SetNextItemWidth(categoriesWidth);
    ImGui::Combo("##categories", &activeCategory, categories, categoryCount);
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
         Application::get().getMessages()) {
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
