// ReSharper disable CppRedundantCastExpression,CppRedundantParentheses
#include "layer/imgui/imguilayer.hpp"

#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/light.hpp"
#include "version.hpp"

#include <algorithm>
#include <optional>
#include <ranges>

namespace {
constexpr ImColor darkDebugColor{ .3F, .8F, .8F, 1.F };
constexpr ImColor darkErrorColor{ .7F, .3F, 0.3F, 1.F };
constexpr ImColor darkWarnColor{ .8F, .8F, 0.3F, 1.F };
constexpr std::string_view cameraName = "maze";
constexpr std::array categories = { "categories", "app", "sponge", "opengl" };
constexpr std::array logLevels = {
    SPDLOG_LEVEL_NAME_TRACE.data(), SPDLOG_LEVEL_NAME_DEBUG.data(),
    SPDLOG_LEVEL_NAME_INFO.data(),  SPDLOG_LEVEL_NAME_WARNING.data(),
    SPDLOG_LEVEL_NAME_ERROR.data(), SPDLOG_LEVEL_NAME_CRITICAL.data(),
    SPDLOG_LEVEL_NAME_OFF.data()
};

constexpr ImGuiWindowFlags windowFlags =
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

constexpr ImGuiTableFlags tableFlags =
    ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX;

constexpr ImVec2 compactSpacing{ 4, 1 };
constexpr float appInfoWidth = 376.F;
constexpr float appInfoHeight = 656.F;
constexpr float logHeight = 220.F;
}  // namespace

namespace game::layer::imgui {
bool ImGuiLayer::hasAppInfoMenu = true;
bool ImGuiLayer::hasLogMenu = true;
bool ImGuiLayer::hasVsync = true;
bool ImGuiLayer::isFullscreen = false;

ImGuiLayer::ImGuiLayer() : Layer("imgui") {
    // nothing
}

void ImGuiLayer::onImGuiRender() {
    const auto window = Maze::get().window;
    const auto width = static_cast<float>(window->getWidth());
    const auto height = static_cast<float>(window->getHeight());

    updateState();
    showMenu();

    if (hasAppInfoMenu) {
        showAppInfoWindow(width);
    }

    if (hasLogMenu) {
        showLogWindow(width, height);
    }
}

void ImGuiLayer::updateState() {
    hasVsync = Maze::get().hasVerticalSync();
    isFullscreen = Maze::get().isFullscreen();
}

void ImGuiLayer::showAppInfoWindow(const float width) {
    ImGui::SetNextWindowPos(
        { width - appInfoWidth,
          ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2 });
    ImGui::SetNextWindowSize({ appInfoWidth, appInfoHeight });

    if (ImGui::Begin("App Info", &hasAppInfoMenu, windowFlags)) {
        showInfoSection();
        showSettingsSection();
        showLightsSection();
        showShadowMapSection();
        showResourcesSection();
        ImGui::End();
    }
}

void ImGuiLayer::showInfoSection() {
    if (!ImGui::CollapsingHeader("Info##Header",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    const auto& io = ImGui::GetIO();
    const auto window = Maze::get().window;
    const std::string appInfo =
        fmt::format("{} {} ({})", project_name.c_str(), project_version.c_str(),
                    git_sha.c_str());

    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", appInfo.c_str());
    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.F / io.Framerate,
                io.Framerate);

    const std::string resolution =
        fmt::format("{}x{}", window->getWidth(), window->getHeight());

    if (ImGui::BeginTable("##ResolutionTable", 2, tableFlags)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Resolution");
        ImGui::TableNextColumn();
        ImGui::Text("%s", resolution.c_str());
        ImGui::EndTable();
    }

    if (ImGui::BeginTable("##CameraTable", 2, tableFlags)) {
        const auto camera = ResourceManager::getGameCamera(cameraName.data());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("FOV");
        ImGui::TableNextColumn();
        ImGui::Text("%.0f", camera->getFov());

        ImGui::EndTable();
    }
}

void ImGuiLayer::showSettingsSection() {
    if (!ImGui::CollapsingHeader("Settings##Header")) {
        return;
    }

    showAppSettingsTable();
}

void ImGuiLayer::showAppSettingsTable() {
    if (ImGui::BeginTable("##AppTable", 2, tableFlags)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Vertical Sync");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##verticalsync", &hasVsync)) {
            Maze::get().setVerticalSync(hasVsync);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Full Screen");
        ImGui::TableNextColumn();
        if (ImGui::Checkbox("##fullscreen", &isFullscreen)) {
            Maze::get().toggleFullscreen();
        }

        ImGui::EndTable();
    }
}

void ImGuiLayer::showLightsSection() {
    if (!ImGui::CollapsingHeader("Lights##Header",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    if (ImGui::BeginTabBar("LightsTabBar")) {
        if (ImGui::BeginTabItem("Point##Tab", nullptr,
                                ImGui::IsWindowAppearing()
                                    ? ImGuiTabItemFlags_SetSelected
                                    : ImGuiTabItemFlags_None)) {
            showPointLightControls();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Directional##Tab")) {
            showDirectionalLightControls();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void ImGuiLayer::showDirectionalLightControls() {
    if (ImGui::BeginTable("DirectionalLights##Table", 2, tableFlags)) {
        const auto mazeLayer = Maze::get().getMazeLayer();
        auto directional = mazeLayer->getDirectionalLightEnabled();
        auto castShadow = mazeLayer->getDirectionalLightCastsShadow();
        auto bias = mazeLayer->getShadowBias();

        showTableRow([&] {
            ImGui::Text("Enable");
            ImGui::TableNextColumn();

            if (ImGui::Checkbox("##directionalenable", &directional)) {
                mazeLayer->setDirectionalLightEnabled(directional);
            }
        });

        static auto colorEditFlags =
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
        auto dirColor = mazeLayer->getDirectionalLightColor();
        ImVec4 color = ImVec4(dirColor.r, dirColor.g, dirColor.b, 1.F);
        showTableRow([&] {
            ImGui::Text("Color");
            ImGui::TableNextColumn();
            if (ImGui::ColorEdit4("##directionalcolor",
                                  reinterpret_cast<float*>(&color),
                                  colorEditFlags)) {
                mazeLayer->setDirectionalLightColor(
                    glm::vec3(color.x, color.y, color.z));
            }
        });

        auto dirDirection = mazeLayer->getDirectionalLightDirection();
        float direction[3] = { dirDirection.x, dirDirection.y, dirDirection.z };
        showTableRow([&] {
            ImGui::Text("Direction");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::InputFloat3("##directionalposition", direction)) {
                mazeLayer->setDirectionalLightDirection(
                    glm::vec3(direction[0], direction[1], direction[2]));
            }
        });

        showTableRow([&] {
            ImGui::Text("Cast Shadow");
            ImGui::TableNextColumn();

            if (ImGui::Checkbox("##directionalshadow", &castShadow)) {
                mazeLayer->setDirectionalLightCastsShadow(castShadow);
            }
        });

        constexpr float step = .001F;

        showTableRow([&] {
            ImGui::Text("Shadow Bias");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::InputScalar("##directionalbias", ImGuiDataType_Float,
                                   &bias, &step)) {
                mazeLayer->setShadowBias(bias);
            }
        });

        showTableRow([&] {
            ImGui::Text("Shadow Map Size");
            ImGui::TableNextColumn();
            const auto width = mazeLayer->getShadowMapWidth();
            const auto height = mazeLayer->getShadowMapHeight();
            ImGui::Text("%ux%u", width, height);
        });

        ImGui::EndTable();
    }
}

void ImGuiLayer::showPointLightControls() {
    const auto mazeLayer = Maze::get().getMazeLayer();
    auto numLights = mazeLayer->getNumLights();
    int32_t attenuationIndex = mazeLayer->getAttenuationIndex();
    if (ImGui::SliderInt("Lights ", &numLights, 1, 6)) {
        mazeLayer->setNumLights(numLights);
    }

    showAttenuationSlider(attenuationIndex);

    if (ImGui::BeginTable(
            "PointLights##Table", 1,
            ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX)) {
        auto metallic = mazeLayer->isMetallic();
        auto ambientStrength = mazeLayer->getAmbientStrength();
        auto roughness = mazeLayer->getRoughness();
        auto ambientOcclusion = mazeLayer->getAmbientOcclusion();

        showTableRow([&] {
            if (ImGui::Checkbox("Metallic", &metallic)) {
                mazeLayer->setMetallic(metallic);
            }
        });

        showTableRow([&] {
            if (ImGui::SliderFloat("Ambient Strength", &ambientStrength, 0.F,
                                   1.F, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                mazeLayer->setAmbientStrength(ambientStrength);
            }
        });

        showTableRow([&] {
            if (ImGui::SliderFloat("Roughness", &roughness, .089F, 1.F, "%.3f",
                                   ImGuiSliderFlags_AlwaysClamp)) {
                mazeLayer->setRoughness(roughness);
            }
        });

        showTableRow([&] {
            if (ImGui::SliderFloat("Ambient Occlusion", &ambientOcclusion, 0.F,
                                   1.F, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                mazeLayer->setAmbientOcclusion(ambientOcclusion);
            }
        });

        ImGui::EndTable();
    }
}

void ImGuiLayer::showAttenuationSlider(int32_t& attenuationIndex) {
    auto attenuation = scene::Light::getAttenuationFromIndex(attenuationIndex);
    const std::string label =
        fmt::format("{:3.0f} [{:1.1f}, {:1.3f}, {:1.4f}]", attenuation.x,
                    attenuation.y, attenuation.z, attenuation.w);

    if (ImGui::SliderInt("Attenuation", &attenuationIndex, 0, 10,
                         label.c_str())) {
        Maze::get().getMazeLayer()->setAttenuationIndex(attenuationIndex);
    }
}

void ImGuiLayer::showShadowMapSection() {
    if (ImGui::CollapsingHeader("Shadow Map")) {
        ImGui::Image(Maze::get().getMazeLayer()->getDepthMapTextureId(),
                     ImVec2(appInfoWidth * .85F, appInfoWidth * .85F));
    }
}

void ImGuiLayer::showResourcesSection() {
    if (!ImGui::CollapsingHeader("Resources")) {
        return;
    }

    showResourceTree("Fonts", [] { showFontsTable(); });
    showResourceTree("Layers", [] {
        auto* const layerStack = Maze::get().getLayerStack();
        showLayersTable(layerStack);
    });
    showResourceTree("Models", [] { showModelsTable(); });
    showResourceTree("Shaders", [] { showShadersTable(); });
    showResourceTree("Textures", [] { showTexturesTable(); });
}

void ImGuiLayer::showLogWindow(const float width, const float height) {
    ImGui::SetNextWindowPos({ 0.F, height - logHeight });
    ImGui::SetNextWindowSize({ width, logHeight });

    if (ImGui::Begin("Logging", &hasLogMenu,
                     windowFlags | ImGuiWindowFlags_NoScrollbar)) {
        showLogging();
        ImGui::End();
    } else {
        hasLogMenu = false;
    }
}

float ImGuiLayer::getLogSelectionMaxWidth(
    const std::span<const char* const> list) {
    float maxWidth = 0;
    for (const auto* item : list) {
        const auto width = ImGui::CalcTextSize(item).x;
        maxWidth = std::max(width, maxWidth);
    }

    return maxWidth + (ImGui::GetStyle().FramePadding.x * 2) +
           ImGui::GetFrameHeight();
}

void ImGuiLayer::showMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                Maze::get().exit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Vertical Sync", nullptr, hasVsync)) {
                hasVsync = !hasVsync;
                Maze::get().setVerticalSync(hasVsync);
            }

            if (ImGui::MenuItem("Full Screen", nullptr, isFullscreen)) {
                isFullscreen = !isFullscreen;
                Maze::get().toggleFullscreen();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("App Info", nullptr, hasAppInfoMenu)) {
                hasAppInfoMenu = !hasAppInfoMenu;
            }
            if (ImGui::MenuItem("Logging", nullptr, hasLogMenu)) {
                hasLogMenu = !hasLogMenu;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void ImGuiLayer::showFontsTable() {
    const auto fonts =
        sponge::platform::opengl::renderer::ResourceManager::getFonts();
    showSimpleTable("fontsTable", fonts);
}

void ImGuiLayer::showLayersTable(sponge::layer::LayerStack* const layerStack) {
    const auto activeColor = ImGui::GetColorU32(ImVec4(.3F, .7F, .3F, .35F));
    const auto inactiveColor = ImGui::GetColorU32(ImVec4(.5F, .5F, .3F, .3F));

    if (ImGui::BeginTable("layerTable", 1)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, compactSpacing);

        for (const auto& layer : std::ranges::reverse_view(*layerStack)) {
            ImGui::TableNextRow();

            const ImU32 cellBgColor =
                layer->isActive() ? activeColor : inactiveColor;
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, cellBgColor);

            ImGui::TableNextColumn();
            ImGui::Text("%s", layer->getName().c_str());
        }

        ImGui::PopStyleVar();
        ImGui::EndTable();
    }
}

void ImGuiLayer::showModelsTable() {
    if (ImGui::BeginTable("modelsTable", 1)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, compactSpacing);

        const auto models =
            sponge::platform::opengl::renderer::ResourceManager::getModels();

        for (const auto& key : models | std::views::keys) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const auto& model = models.at(key);
            ImGui::Text("%s", fmt::format("{:10s} vertices {:7d} indices {:7d}",
                                          key, model->getNumVertices(),
                                          model->getNumIndices())
                                  .c_str());
        }

        ImGui::PopStyleVar();
        ImGui::EndTable();
    }
}

void ImGuiLayer::showShadersTable() {
    const auto shaders =
        sponge::platform::opengl::renderer::ResourceManager::getShaders();
    showSimpleTable("shaderTable", shaders);
}

void ImGuiLayer::showTexturesTable() {
    if (ImGui::BeginTable("textureTable", 1)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, compactSpacing);

        const auto textures =
            sponge::platform::opengl::renderer::ResourceManager::getTextures();

        for (const auto& key : textures | std::views::keys) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", key.c_str());
            ImGui::Image(textures.at(key)->getId(), ImVec2(200, 200));
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

    showLogControls(filter, logLevelWidth, categoriesWidth, activeLogLevel,
                    activeCategory);
    showLogMessages(filter, activeLogLevel, activeCategory);
}

void ImGuiLayer::showLogControls(ImGuiTextFilter& filter,
                                 const float logLevelWidth,
                                 const float categoriesWidth,
                                 spdlog::level::level_enum& activeLogLevel,
                                 int& activeCategory) {
    ImGui::SetNextItemWidth(logLevelWidth);
    ImGui::Combo("##activeLogLevel", reinterpret_cast<int*>(&activeLogLevel),
                 logLevels.data(), static_cast<int>(logLevels.size()));
    ImGui::SameLine();

    ImGui::SetNextItemWidth(categoriesWidth);
    ImGui::Combo("##categories", &activeCategory, categories.data(),
                 static_cast<int>(categories.size()));
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
}

void ImGuiLayer::showLogMessages(const ImGuiTextFilter& filter,
                                 const spdlog::level::level_enum activeLogLevel,
                                 const int activeCategory) {
    ImGui::BeginChild("LogTextView",
                      ImVec2(0, -ImGui::GetStyle().ItemSpacing.y),
                      ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

    for (const auto& [message, loggerName, level] : Maze::get().getMessages()) {
        if (!shouldShowLogMessage(message, loggerName, level, filter,
                                  activeLogLevel, activeCategory)) {
            continue;
        }

        renderLogMessage(message, level);
    }

    ImGui::PopStyleVar();

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.F);
    }

    ImGui::EndChild();
}

bool ImGuiLayer::shouldShowLogMessage(
    const std::string& message, const std::string& loggerName,
    const spdlog::level::level_enum level, const ImGuiTextFilter& filter,
    const spdlog::level::level_enum activeLogLevel, const int activeCategory) {
    if (level < activeLogLevel) {
        return false;
    }

    if (activeCategory > 0) {
        if (const auto category = std::string(categories[activeCategory]);
            !isCaseInsensitiveEqual(loggerName, category)) {
            return false;
        }
    }

    return filter.PassFilter(message.c_str());
}

void ImGuiLayer::renderLogMessage(const std::string& message,
                                  const spdlog::level::level_enum level) {
    const auto color = getLogLevelColor(level);
    const bool hasColor = color.has_value();

    if (hasColor) {
        ImGui::PushStyleColor(ImGuiCol_Text, color.value());
    }

    ImGui::TextUnformatted(message.c_str());

    if (hasColor) {
        ImGui::PopStyleColor();
    }
}

std::optional<ImVec4> ImGuiLayer::getLogLevelColor(
    const spdlog::level::level_enum level) {
    switch (level) {
        case spdlog::level::debug:
            return darkDebugColor;
        case spdlog::level::warn:
            return darkWarnColor;
        case spdlog::level::err:
            return darkErrorColor;
        default:
            return std::nullopt;
    }
}

bool ImGuiLayer::isCaseInsensitiveEqual(const std::string& str1,
                                        const std::string& str2) {
    return str1.size() == str2.size() &&
           std::equal(str1.begin(), str1.end(), str2.begin(),
                      [](auto a, auto b) {
                          return std::tolower(a) == std::tolower(b);
                      });
}
}  // namespace game::layer::imgui
