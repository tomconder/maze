#include "imguilayer.h"
#include "imgui.h"
#include "version.h"

constexpr ImColor LVL_DBG_CLR{ .3F, .8F, .8F, 1.F };
constexpr ImColor LVL_ERR_CLR{ .7F, .3F, 0.3F, 1.F };
constexpr ImColor LVL_LOG_CLR{ 1.F, 1.F, 1.F, 1.F };
constexpr ImColor LVL_WRN_CLR{ .8F, .8F, 0.3F, 1.F };

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

    const auto width = static_cast<float>(sponge::SDLEngine::get().getWidth());
    const auto height =
        static_cast<float>(sponge::SDLEngine::get().getHeight());

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

    if (ImGui::Begin("Logging", nullptr, windowFlags)) {
        showLogging();
        ImGui::End();
    }
}

void ImGuiLayer::showLayersTable(sponge::layer::LayerStack* const layerStack) {
    const auto activeColor = ImGui::GetColorU32(ImVec4(.3F, .7F, .3F, .35F));
    const auto inactiveColor = ImGui::GetColorU32(ImVec4(.5F, .5F, .3F, .3F));

    if (ImGui::BeginTable("layerTable", 1)) {
        for (auto layer = layerStack->rbegin(); layer != layerStack->rend();
             ++layer) {
            ImGui::TableNextRow();

            const ImU32 cellBgColor =
                (*layer)->isActive() ? activeColor : inactiveColor;

            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, cellBgColor);

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", (*layer)->getName().c_str());
        }

        ImGui::EndTable();
    }
}

void ImGuiLayer::showLogging() {
    static ImGuiTextFilter filter;
    static bool enableAutoScrolling = true;

    ImGui::Checkbox("Auto-scroll", &enableAutoScrolling);
    ImGui::SameLine();

    if (ImGui::Button("Clear")) {
        sponge::SDLEngine::get().clearMessages();
    }
    ImGui::SameLine();

    static const auto logSelectionWidth = []() -> float {
        float LongestTextWidth = 0;
        for (auto LogLevelText : logLevels) {
            auto TextWidth = ImGui::CalcTextSize(LogLevelText).x;
            if (TextWidth > LongestTextWidth)
                LongestTextWidth = TextWidth;
        }

        return LongestTextWidth + ImGui::GetStyle().FramePadding.x * 2 +
               ImGui::GetFrameHeight();
    }();

    auto comboBoxRightAlignment =
        ImGui::GetWindowSize().x -
        (logSelectionWidth + ImGui::GetStyle().WindowPadding.x);

    auto activeLogLevel = spdlog::get_level();
    ImGui::SetNextItemWidth(logSelectionWidth);
    ImGui::SameLine(comboBoxRightAlignment);
    if (ImGui::Combo("##activeLogLevel",
                     reinterpret_cast<int*>(&activeLogLevel), logLevels.data(),
                     std::size(logLevels))) {
        spdlog::set_level(activeLogLevel);
    }

    filter.Draw("Filter",
                ImGui::GetWindowSize().x -
                    (logSelectionWidth + ImGui::GetStyle().WindowPadding.x * 2 +
                     ImGui::GetStyle().FramePadding.x));

    ImGui::Separator();
    ImGui::BeginChild("LogTextView", ImVec2(0, 0), 0,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar |
                          ImGuiWindowFlags_AlwaysHorizontalScrollbar);

    for (const auto& [message, level] :
         sponge::SDLEngine::get().getMessages()) {
        if (level < activeLogLevel) {
            continue;
        }

        switch (level) {
            case spdlog::level::debug:
                ImGui::TextColored(LVL_DBG_CLR, "%s", message.c_str());
                break;
            case spdlog::level::warn:
                ImGui::TextColored(LVL_WRN_CLR, "%s", message.c_str());
                break;
            case spdlog::level::err:
                ImGui::TextColored(LVL_ERR_CLR, "%s", message.c_str());
                break;
            default:
                ImGui::TextColored(LVL_LOG_CLR, "%s", message.c_str());
                break;
        }
    }

    ImGui::EndChild();
}
