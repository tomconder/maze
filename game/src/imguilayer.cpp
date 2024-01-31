#include "imguilayer.h"
#include "imgui.h"
#include "version.h"

constexpr ImColor DARK_DEBUG_COLOR{ .3F, .8F, .8F, 1.F };
constexpr ImColor DARK_ERROR_COLOR{ .7F, .3F, 0.3F, 1.F };
constexpr ImColor DARK_NORMAL_COLOR{ 1.F, 1.F, 1.F, 1.F };
constexpr ImColor DARK_WARN_COLOR{ .8F, .8F, 0.3F, 1.F };

const std::vector logLevels{
    SPDLOG_LEVEL_NAME_TRACE.data(), SPDLOG_LEVEL_NAME_DEBUG.data(),
    SPDLOG_LEVEL_NAME_INFO.data(),  SPDLOG_LEVEL_NAME_WARNING.data(),
    SPDLOG_LEVEL_NAME_ERROR.data(), SPDLOG_LEVEL_NAME_CRITICAL.data(),
    SPDLOG_LEVEL_NAME_OFF.data()
};

ImGuiLayer::ImGuiLayer() : Layer("imgui") {
    // nothing
}

void ImGuiLayer::onImGuiRender() {
    const auto& io = ImGui::GetIO();

    const auto width =
        static_cast<float>(sponge::SDLEngine::get().getWindowWidth());
    const auto height =
        static_cast<float>(sponge::SDLEngine::get().getWindowHeight());

    ImGui::SetNextWindowPos({ width - 320.F, 0.F });
    ImGui::SetNextWindowSize({ 320.F, 200.F });

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

        if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto* const layerStack = sponge::SDLEngine::get().getLayerStack();
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

float ImGuiLayer::getLogSelectionMaxWidth() {
    float maxWidth = 0;
    for (const auto* level : logLevels) {
        const auto width = ImGui::CalcTextSize(level).x;
        if (width > maxWidth) {
            maxWidth = width;
        }
    }

    return maxWidth + ImGui::GetStyle().FramePadding.x * 2 +
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
    static auto logSelectionWidth = getLogSelectionMaxWidth();
    static spdlog::level::level_enum activeLogLevel = spdlog::get_level();

    if (ImGui::Button("Clear")) {
        sponge::SDLEngine::get().clearMessages();
    }
    ImGui::SameLine();

    ImGui::TextUnformatted("Severity:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(logSelectionWidth);
    ImGui::Combo("##activeLogLevel", reinterpret_cast<int*>(&activeLogLevel),
                 logLevels.data(), std::size(logLevels));
    ImGui::SameLine();

    ImGui::TextUnformatted("Filter:");
    ImGui::SameLine();
    filter.Draw("##filter", -1);

    ImGui::Separator();
    ImGui::BeginChild(
        "LogTextView", ImVec2(0, -ImGui::GetStyle().ItemSpacing.y),
                      ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
    ImVec4 color;

    for (const auto& [message, level] :
         sponge::SDLEngine::get().getMessages()) {
        if (level < activeLogLevel) {
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
