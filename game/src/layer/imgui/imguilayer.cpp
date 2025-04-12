#include "imguilayer.hpp"
#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/pointlight.hpp"
#include "version.hpp"
#include <imgui.h>
#include <algorithm>
#include <ranges>

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

namespace game::layer::imgui {
bool ImGuiLayer::hasAppInfoMenu = true;
bool ImGuiLayer::hasLogMenu = true;
bool ImGuiLayer::hasVsync = true;
bool ImGuiLayer::isFullscreen = false;

ImGuiLayer::ImGuiLayer() : Layer("imgui") {
    // nothing
}

void ImGuiLayer::onImGuiRender() {
    const auto& io = ImGui::GetIO();

    const auto window = Maze::get().window;

    const auto width = static_cast<float>(window->getWidth());
    const auto height = static_cast<float>(window->getHeight());

    hasVsync = game::Maze::get().hasVerticalSync();
    isFullscreen = game::Maze::get().isFullscreen();

    const auto mazeLayer = Maze::get().getMazeLayer();

    auto ambientOcclusion = mazeLayer->getAmbientOcclusion();
    auto ambientStrength = mazeLayer->getAmbientStrength();
    auto attenuationIndex = mazeLayer->getAttenuationIndex();
    auto metallic = mazeLayer->isMetallic();
    auto numLights = mazeLayer->getNumLights();
    auto roughness = mazeLayer->getRoughness();

    showMenu();

    constexpr auto windowFlags =
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    if (hasAppInfoMenu) {
        ImGui::SetNextWindowPos(
            { width - 376.F,
              ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2 });
        ImGui::SetNextWindowSize({ 376.F, 656.F });

        const std::string appInfo =
            fmt::format("{} {} ({})", project_name.c_str(),
                        project_version.c_str(), git_sha.c_str());

        if (ImGui::Begin(appInfo.c_str(), nullptr, windowFlags)) {
            if (ImGui::CollapsingHeader("App Info##Header"),
                ImGuiTreeNodeFlags_DefaultOpen) {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Resolution: %dx%d", window->getWidth(),
                            window->getHeight());
                ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
                            1000.F / io.Framerate, io.Framerate);

                ImGui::BeginTable(
                    "##CameraTable", 2,
                    ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX);

                const auto camera = ResourceManager::getGameCamera(cameraName);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("FOV");
                ImGui::TableNextColumn();
                ImGui::Text("%.0f", camera->getFov());

                ImGui::EndTable();

                ImGui::SeparatorText("Settings##App");

                ImGui::BeginTable(
                    "##AppTable", 2,
                    ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Vertical Sync");
                ImGui::TableNextColumn();
                if (ImGui::Checkbox("##vertical-sync", &hasVsync)) {
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

            if (ImGui::CollapsingHeader("Lights##Header",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::SliderInt("Lights ", &numLights, 1, 6)) {
                    mazeLayer->setNumLights(numLights);
                }

                auto attenuation =
                    PointLight::getAttenuationFromIndex(attenuationIndex);
                std::string label = fmt::format(
                    "{:3.0f} [{:1.1f}, {:1.3f}, {:1.4f}]", attenuation.x,
                    attenuation.y, attenuation.z, attenuation.w);

                if (ImGui::SliderInt("Attentuation", &attenuationIndex, 0, 10,
                                     label.c_str())) {
                    mazeLayer->setAttenuationIndex(attenuationIndex);
                }

                ImGui::SeparatorText("PBR");
                ImGui::BeginTable(
                    "PBR##Table", 1,
                    ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Checkbox("Metallic", &metallic)) {
                    mazeLayer->setMetallic(metallic);
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::SliderFloat("Ambient Strength", &ambientStrength,
                                       0.F, 1.F, "%.3f",
                                       ImGuiSliderFlags_AlwaysClamp)) {
                    mazeLayer->setAmbientStrength(ambientStrength);
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::SliderFloat("Roughness", &roughness, 0.F, 1.F,
                                       "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                    mazeLayer->setRoughness(roughness);
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::SliderFloat("Ambient Occlusion", &ambientOcclusion,
                                       0.F, 1.F, "%.3f",
                                       ImGuiSliderFlags_AlwaysClamp)) {
                    mazeLayer->setAmbientOcclusion(ambientOcclusion);
                }

                ImGui::EndTable();
            }

            if (ImGui::CollapsingHeader("Resources")) {
                if (ImGui::TreeNode("Fonts")) {
                    showFontsTable();
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Layers")) {
                    auto* const layerStack = Maze::get().getLayerStack();
                    showLayersTable(layerStack);
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Models")) {
                    showModelsTable();
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Shaders")) {
                    showShadersTable();
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Textures")) {
                    showTexturesTable();
                    ImGui::TreePop();
                }
            }

            ImGui::End();
        }
    }

    if (hasLogMenu) {
        ImGui::SetNextWindowPos({ 0.F, height - 220.F });
        ImGui::SetNextWindowSize({ width, 220.F });

        if (ImGui::Begin("Logging", nullptr,
                         windowFlags | ImGuiWindowFlags_NoScrollbar)) {
            showLogging();
            ImGui::End();
        }
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
    if (ImGui::BeginTable("fontsTable", 1)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

        const auto fonts =
            sponge::platform::opengl::renderer::ResourceManager::getFonts();

        for (const auto& key : fonts | std::views::keys) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", key.c_str());
        }

        ImGui::PopStyleVar();

        ImGui::EndTable();
    }
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

            ImGui::TableNextColumn();
            ImGui::Text("%s", (*layer)->getName().c_str());
        }

        ImGui::PopStyleVar();

        ImGui::EndTable();
    }
}

void ImGuiLayer::showModelsTable() {
    if (ImGui::BeginTable("modelsTable", 1)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

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
    if (ImGui::BeginTable("shaderTable", 1)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

        const auto shaders =
            sponge::platform::opengl::renderer::ResourceManager::getShaders();

        for (const auto& key : shaders | std::views::keys) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", key.c_str());
        }

        ImGui::PopStyleVar();

        ImGui::EndTable();
    }
}

void ImGuiLayer::showTexturesTable() {
    if (ImGui::BeginTable("textureTable", 1)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

        const auto textures =
            sponge::platform::opengl::renderer::ResourceManager::getTextures();

        for (const auto& key : textures | std::views::keys) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", key.c_str());
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

    for (const auto& [message, loggerName, level] : Maze::get().getMessages()) {
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

        bool hasColor = false;
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
}  // namespace game::layer::imgui
