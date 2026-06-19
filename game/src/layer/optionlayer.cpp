#include "layer/optionlayer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <yoga/Yoga.h>

#include "core/settings.hpp"
#include "event/event.hpp"
#include "maze.hpp"
#include "resourcemanager.hpp"
#include "scene/orthocamera.hpp"
#include "sponge.hpp"
#include "ui/button.hpp"
#include "ui/checkbox.hpp"
#include "ui/menufontsize.hpp"
#include "ui/selectlist.hpp"

namespace {
struct AspectRatioFilter {
    std::string_view label;
    uint32_t         numerator;
    uint32_t         denominator;
    bool             approximate = false;
};

constexpr auto aspectRatioFilters = std::to_array<AspectRatioFilter>({
    { .label = "4:3", .numerator = 4, .denominator = 3 },
    { .label = "5:3", .numerator = 5, .denominator = 3 },
    { .label = "5:4", .numerator = 5, .denominator = 4 },
    { .label = "16:9", .numerator = 16, .denominator = 9 },
    { .label       = "~16:9",
      .numerator   = 16,
      .denominator = 9,
      .approximate = true },
    { .label = "16:10", .numerator = 16, .denominator = 10 },
    { .label = "25:16", .numerator = 25, .denominator = 16 },
});

constexpr std::string_view returnMessage = "Return";
constexpr std::string_view applyMessage  = "Apply";

constexpr std::array<uint32_t, 4> shadowResolutions = { 512, 1024, 2048, 4096 };

constexpr std::array<float, 3> bloomThresholds  = { 0.6F, 0.8F, 0.95F };
constexpr std::array<float, 4> bloomIntensities = { 0.5F, 1.0F, 2.0F, 3.0F };

constexpr std::string_view cameraName = "intro";
constexpr std::string_view fontName   = "inter";
constexpr std::string_view fontPath   = "/fonts/inter.ttf";

constexpr glm::vec4 backgroundColor    = { 0.F, 0.F, 0.F, 1.F };
constexpr glm::vec4 buttonColor        = { 0.F, 0.F, 0.F, 0.F };
constexpr glm::vec4 hoverColor         = { 0.84F, 0.84F, 0.84F, 0.14F };
constexpr glm::vec3 textColor          = { 1.F, 1.F, 1.F };
constexpr glm::vec3 arrowDisabledColor = { 0.4F, 0.4F, 0.4F };
constexpr glm::vec4 textHoverColor     = { 0.84F, 0.04F, 0.04F, 0.14F };

constexpr glm::vec4 dotColorDefault = { 0.25F, 0.25F, 0.25F, 1.F };
constexpr glm::vec4 dotColorCurrent = { 1.F, 1.F, 1.F, 1.F };
constexpr glm::vec4 dotColorDisplay = { 0.65F, 0.65F, 0.65F, 1.F };

uint32_t        fontSize            = 48;
constexpr float textMarginLeft      = 26.F;
constexpr float cornerRadius        = 12.F;
constexpr float selectedBorderWidth = 3.F;

std::vector<sponge::core::Resolution> availableResolutions;
std::vector<sponge::core::Resolution> filteredResolutions;
std::vector<AspectRatioFilter>        validAspectRatioFilters;
std::optional<size_t>                 currentAspectRatioIndex;
std::optional<size_t>                 currentResolutionIndex;

bool matchesAspectRatio(const AspectRatioFilter& filter, const uint32_t width,
                        const uint32_t height) {
    const auto fg    = std::gcd(filter.numerator, filter.denominator);
    const auto g     = std::gcd(width, height);
    const bool exact = width / g == filter.numerator / fg &&
                       height / g == filter.denominator / fg;
    if (filter.approximate) {
        const float ratio =
            static_cast<float>(width) / static_cast<float>(height);
        const float target = static_cast<float>(filter.numerator) /
                             static_cast<float>(filter.denominator);
        return !exact && std::abs(ratio - target) / target <= 0.01F;
    }
    return exact;
}

bool hasMatchingResolution(
    const AspectRatioFilter&                     filter,
    const std::vector<sponge::core::Resolution>& resolutions) {
    for (const auto& res : resolutions) {
        if (matchesAspectRatio(filter, res.width, res.height)) {
            return true;
        }
    }
    return false;
}

size_t findAspectRatioIndex(const uint32_t width, const uint32_t height) {
    const auto g  = std::gcd(width, height);
    const auto rw = width / g;
    const auto rh = height / g;
    auto       it =
        std::find_if(validAspectRatioFilters.begin(),
                     validAspectRatioFilters.end(), [&](const auto& f) {
                         const auto fg = std::gcd(f.numerator, f.denominator);
                         return !f.approximate && rw == f.numerator / fg &&
                                rh == f.denominator / fg;
                     });
    if (it != validAspectRatioFilters.end()) {
        return static_cast<size_t>(
            std::distance(validAspectRatioFilters.begin(), it));
    }
    it = std::find_if(
        validAspectRatioFilters.begin(), validAspectRatioFilters.end(),
        [&](const auto& f) { return matchesAspectRatio(f, width, height); });
    return it != validAspectRatioFilters.end() ?
               static_cast<size_t>(
                   std::distance(validAspectRatioFilters.begin(), it)) :
               0;
}

std::tuple<float, float, float, float> getNodeLayout(const YGNodeRef node,
                                                     const float     offsetX,
                                                     const float     offsetY) {
    return { offsetX + YGNodeLayoutGetLeft(node),
             offsetY + YGNodeLayoutGetTop(node), YGNodeLayoutGetWidth(node),
             YGNodeLayoutGetHeight(node) };
}

std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> menuFont;

// max display width across all aspect-ratio labels and resolution strings
float computeMaxCycleValueWidth() {
    float width = std::accumulate(
        validAspectRatioFilters.begin(), validAspectRatioFilters.end(), 0.F,
        [](const float acc, const auto& f) {
            return std::max(acc, static_cast<float>(
                                     menuFont->getLength(f.label, fontSize)));
        });
    return std::accumulate(
        availableResolutions.begin(), availableResolutions.end(), width,
        [](const float acc, const auto& res) {
            return std::max(
                acc,
                static_cast<float>(menuFont->getLength(
                    fmt::format("{} × {}", res.width, res.height), fontSize)));
        });
}

void saveVideoSettings(const uint32_t width, const uint32_t height,
                       const bool fullscreen, const bool vsync, const bool fxaa,
                       const uint32_t shadowRes, const bool bloomEnabled,
                       const float bloomThreshold, const float bloomIntensity) {
    using sponge::core::Settings;
    Settings::set("video.width", width);
    Settings::set("video.height", height);
    Settings::set("video.fullscreen", fullscreen);
    Settings::set("video.vsync", vsync);
    Settings::set("video.fxaa", fxaa);
    Settings::set("video.shadowRes", shadowRes);
    Settings::set("video.bloomEnabled", bloomEnabled);
    Settings::set("video.bloomThreshold", std::to_string(bloomThreshold));
    Settings::set("video.bloomIntensity", std::to_string(bloomIntensity));
    Settings::save();
}

std::unique_ptr<game::ui::Button> returnButton;

YGNodeRef antiAliasingNode   = nullptr;
YGNodeRef aspectRatioNode    = nullptr;
YGNodeRef menuBackgroundNode = nullptr;
YGNodeRef menuNode           = nullptr;
YGNodeRef resolutionNode     = nullptr;
YGNodeRef shadowQualityNode  = nullptr;
YGNodeRef verticalSyncNode   = nullptr;
YGNodeRef fullScreenNode     = nullptr;
YGNodeRef bloomEnabledNode   = nullptr;
YGNodeRef bloomThresholdNode = nullptr;
YGNodeRef bloomIntensityNode = nullptr;
YGNodeRef returnNode         = nullptr;
YGNodeRef rootNode           = nullptr;
YGNodeRef titleNode          = nullptr;

std::optional<size_t> currentShadowResIndex;

std::unique_ptr<sponge::platform::opengl::scene::Quad> quad;

std::shared_ptr<game::scene::OrthoCamera> orthoCamera;
}  // namespace

namespace game::layer {
using sponge::event::Event;
using sponge::event::EventDispatcher;
using sponge::event::MouseButtonPressedEvent;
using sponge::event::MouseMovedEvent;
using sponge::event::WindowResizeEvent;
using sponge::platform::opengl::renderer::AssetManager;
using sponge::platform::opengl::scene::FontCreateInfo;
using sponge::platform::opengl::scene::Quad;

OptionLayer::OptionLayer() : Layer("options") {}

void OptionLayer::onAttach() {
    const auto fontCreateInfo = FontCreateInfo{ .name = std::string(fontName),
                                                .path = std::string(fontPath) };
    menuFont                  = AssetManager::createFont(fontCreateInfo);

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    quad = std::make_unique<Quad>();

    fontSize = ui::menuFontSizeForWidth(
        static_cast<uint32_t>(orthoCamera->getWidth()));

    returnButton = std::make_unique<ui::Button>(
        ui::ButtonCreateInfo{ .topLeft      = glm::vec2{ 0.F },
                              .bottomRight  = glm::vec2{ 0.F },
                              .message      = std::string(returnMessage),
                              .fontSize     = fontSize,
                              .font         = menuFont,
                              .buttonColor  = buttonColor,
                              .textColor    = textColor,
                              .marginLeft   = 26,
                              .cornerRadius = 12.F,
                              .alignType = ui::ButtonAlignType::LeftAligned });

    for (const auto& shader : { menuFont->getShader(), Quad::getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    rootNode = YGNodeNew();

    titleNode = YGNodeNew();
    YGNodeStyleSetFlexGrow(titleNode, 0.9F);
    YGNodeInsertChild(rootNode, titleNode, 0);

    menuNode = YGNodeNew();
    YGNodeStyleSetFlex(menuNode, 1.F);
    YGNodeStyleSetFlexDirection(menuNode, YGFlexDirectionRow);
    YGNodeInsertChild(rootNode, menuNode, 1);

    menuBackgroundNode = YGNodeNew();
    YGNodeStyleSetMargin(menuBackgroundNode, YGEdgeAll, 10.F);
    YGNodeStyleSetWidthPercent(menuBackgroundNode, 45.F);
    YGNodeInsertChild(menuNode, menuBackgroundNode, 0);

    auto makeMenuNode = [](const YGNodeRef parent, const int index) {
        auto* const child = YGNodeNew();
        YGNodeStyleSetFlex(child, 1.F);
        YGNodeStyleSetMaxHeight(child, 110);
        YGNodeInsertChild(parent, child, index);
        return child;
    };

    aspectRatioNode    = makeMenuNode(menuBackgroundNode, 0);
    resolutionNode     = makeMenuNode(menuBackgroundNode, 1);
    fullScreenNode     = makeMenuNode(menuBackgroundNode, 2);
    verticalSyncNode   = makeMenuNode(menuBackgroundNode, 3);
    antiAliasingNode   = makeMenuNode(menuBackgroundNode, 4);
    shadowQualityNode  = makeMenuNode(menuBackgroundNode, 5);
    bloomEnabledNode   = makeMenuNode(menuBackgroundNode, 6);
    bloomThresholdNode = makeMenuNode(menuBackgroundNode, 7);
    bloomIntensityNode = makeMenuNode(menuBackgroundNode, 8);
    returnNode         = makeMenuNode(menuBackgroundNode, 9);

    availableResolutions = Maze::get().getAvailableResolutions();

    validAspectRatioFilters.clear();
    for (const auto& f : aspectRatioFilters) {
        if (hasMatchingResolution(f, availableResolutions)) {
            validAspectRatioFilters.push_back(f);
        }
    }

    const float maxCycleValueWidth = computeMaxCycleValueWidth();

    const ui::SelectListCreateInfo selectCreateInfo{
        .font               = menuFont,
        .fontSize           = fontSize,
        .textColor          = textColor,
        .arrowDisabledColor = arrowDisabledColor,
        .textMarginLeft     = textMarginLeft,
        .maxValueWidth      = maxCycleValueWidth,
    };
    aspectRatioList   = std::make_unique<ui::SelectList>(selectCreateInfo);
    resolutionList    = std::make_unique<ui::SelectList>(selectCreateInfo);
    shadowQualityList = std::make_unique<ui::SelectList>(selectCreateInfo);

    const ui::CheckboxCreateInfo checkboxCreateInfo{
        .margin = textMarginLeft, .size = static_cast<float>(fontSize)
    };
    antiAliasingCheckbox = std::make_unique<ui::Checkbox>(checkboxCreateInfo);
    fullScreenCheckbox   = std::make_unique<ui::Checkbox>(checkboxCreateInfo);
    verticalSyncCheckbox = std::make_unique<ui::Checkbox>(checkboxCreateInfo);
    bloomEnabledCheckbox = std::make_unique<ui::Checkbox>(checkboxCreateInfo);
    bloomThresholdList   = std::make_unique<ui::SelectList>(selectCreateInfo);
    bloomIntensityList   = std::make_unique<ui::SelectList>(selectCreateInfo);

    bloomThresholdList->setItems({ "Low", "Medium", "High" });
    bloomIntensityList->setItems({ "Low", "Normal", "High", "Extra" });

    std::vector<std::string> arItems;
    arItems.reserve(validAspectRatioFilters.size());
    for (const auto& f : validAspectRatioFilters) {
        arItems.emplace_back(f.label);
    }
    aspectRatioList->setItems(std::move(arItems));

    {
        shadowQualityList->setItems({ "Low", "Normal", "High", "Ultra" });
    }

    {
        const auto currentShadowRes =
            Maze::get().getMazeLayer()->getDirectionalLightShadowMapRes();
        const auto it = std::ranges::find(shadowResolutions, currentShadowRes);
        pendingShadowResIndex =
            it != shadowResolutions.end() ?
                static_cast<int>(it - shadowResolutions.begin()) :
                1;
        shadowQualityList->setSelectedIndex(
            static_cast<size_t>(pendingShadowResIndex));
        currentShadowResIndex = static_cast<size_t>(pendingShadowResIndex);
    }

    const auto window        = Maze::get().getWindow();
    const auto currentWidth  = window->getWidth();
    const auto currentHeight = window->getHeight();

    syncPendingCheckboxState();

    aspectRatioList->setSelectedIndex(
        findAspectRatioIndex(currentWidth, currentHeight));

    filterResolutions();

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());
    recalculateLayout(width, height);
}

void OptionLayer::onDetach() {
    YGNodeFreeRecursive(rootNode);
}

void OptionLayer::onEvent(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.dispatch<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& mouseEvent) {
            return isActive() ? onMouseButtonPressed(mouseEvent) : false;
        });
    dispatcher.dispatch<MouseMovedEvent>(
        [this](const MouseMovedEvent& mouseMovedEvent) {
            return isActive() ? onMouseMoved(mouseMovedEvent) : false;
        });
    dispatcher.dispatch<WindowResizeEvent>(
        [this](const WindowResizeEvent& windowResizeEvent) {
            return onWindowResize(windowResizeEvent);
        });
}

bool OptionLayer::onUpdate(const double elapsedTime) {
    UNUSED(elapsedTime);

    {
        using sponge::input::GameAction;
        auto& mgr =
            sponge::platform::glfw::core::Application::get().getInputManager();
        mgr.setActiveContext(sponge::input::InputContext::Menu);

        {
            const auto& input = mgr.getSnapshot();
            if (!wasActiveLastFrame) {
                syncPendingCheckboxState();
                updateChangeStatus();
                waitForConfirmRelease = input.isHeld(GameAction::MenuConfirm);
            } else if (waitForConfirmRelease &&
                       !input.isHeld(GameAction::MenuConfirm)) {
                waitForConfirmRelease = false;
            }
        }

        if (wasActiveLastFrame) {
            const auto&    input     = mgr.getSnapshot();
            constexpr auto itemCount = static_cast<int>(OptionMenuItem::Count);

            if (input.isActive(GameAction::MenuDown)) {
                selectedItem = static_cast<OptionMenuItem>(
                    (static_cast<int>(selectedItem) + 1) % itemCount);
            }
            if (input.isActive(GameAction::MenuUp)) {
                selectedItem = static_cast<OptionMenuItem>(
                    (static_cast<int>(selectedItem) - 1 + itemCount) %
                    itemCount);
            }
            if (input.isActive(GameAction::MenuLeft)) {
                if (selectedItem == OptionMenuItem::AspectRatio) {
                    aspectRatioList->selectPrev();
                    filterResolutions();
                } else if (selectedItem == OptionMenuItem::Resolution &&
                           !filteredResolutions.empty()) {
                    resolutionList->selectPrev();
                    updateChangeStatus();
                } else if (selectedItem == OptionMenuItem::ShadowQuality) {
                    shadowQualityList->selectPrev();
                    pendingShadowResIndex =
                        static_cast<int>(shadowQualityList->getSelectedIndex());
                    updateChangeStatus();
                } else if (selectedItem == OptionMenuItem::BloomThreshold) {
                    bloomThresholdList->selectPrev();
                    pendingBloomThreshold = static_cast<int>(
                        bloomThresholdList->getSelectedIndex());
                    updateChangeStatus();
                } else if (selectedItem == OptionMenuItem::BloomIntensity) {
                    bloomIntensityList->selectPrev();
                    pendingBloomIntensity = static_cast<int>(
                        bloomIntensityList->getSelectedIndex());
                    updateChangeStatus();
                }
            }
            if (input.isActive(GameAction::MenuRight)) {
                if (selectedItem == OptionMenuItem::AspectRatio) {
                    aspectRatioList->selectNext();
                    filterResolutions();
                } else if (selectedItem == OptionMenuItem::Resolution &&
                           !filteredResolutions.empty()) {
                    resolutionList->selectNext();
                    updateChangeStatus();
                } else if (selectedItem == OptionMenuItem::ShadowQuality) {
                    shadowQualityList->selectNext();
                    pendingShadowResIndex =
                        static_cast<int>(shadowQualityList->getSelectedIndex());
                    updateChangeStatus();
                } else if (selectedItem == OptionMenuItem::BloomThreshold) {
                    bloomThresholdList->selectNext();
                    pendingBloomThreshold = static_cast<int>(
                        bloomThresholdList->getSelectedIndex());
                    updateChangeStatus();
                } else if (selectedItem == OptionMenuItem::BloomIntensity) {
                    bloomIntensityList->selectNext();
                    pendingBloomIntensity = static_cast<int>(
                        bloomIntensityList->getSelectedIndex());
                    updateChangeStatus();
                }
            }
            if (input.isActive(GameAction::MenuBack)) {
                mgr.consumeActive(GameAction::MenuBack);
                clearHoveredItems();
                selectedItem = OptionMenuItem::AspectRatio;
                resetSelectionToCurrentState();
                setActive(false);
            }
            if (!waitForConfirmRelease &&
                input.isActive(GameAction::MenuConfirm)) {
                mgr.consumeActive(GameAction::MenuConfirm);
                if (selectedItem == OptionMenuItem::Return) {
                    if (hasUnappliedChanges) {
                        applyChanges();
                    } else {
                        clearHoveredItems();
                        selectedItem = OptionMenuItem::AspectRatio;
                        resetSelectionToCurrentState();
                        setActive(false);
                    }
                } else if (selectedItem == OptionMenuItem::FullScreen) {
                    pendingFullscreen = !pendingFullscreen;
                    updateChangeStatus();
                } else if (selectedItem == OptionMenuItem::VerticalSync) {
                    pendingVsync = !pendingVsync;
                    updateChangeStatus();
                } else if (selectedItem == OptionMenuItem::AntiAliasing) {
                    pendingFxaa = !pendingFxaa;
                    updateChangeStatus();
                } else if (selectedItem == OptionMenuItem::BloomEnabled) {
                    pendingBloomEnabled = !pendingBloomEnabled;
                    updateChangeStatus();
                }
            }
        }
        wasActiveLastFrame = true;
    }

    for (const auto& shader : { menuFont->getShader(), Quad::getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    const auto width  = static_cast<float>(orthoCamera->getWidth());
    const auto height = static_cast<float>(orthoCamera->getHeight());
    quad->render({ 0.F, 0.F }, { width, height }, backgroundColor);

    auto [rootNodeX, rootNodeY, rootNodeW, rootNodeH] =
        getNodeLayout(rootNode, 0.F, 0.F);
    auto [menuNodeX, menuNodeY, menuNodeW, menuNodeH] =
        getNodeLayout(menuNode, rootNodeX, rootNodeY);
    auto [menuBgX, menuBgY, menuBgW, menuBgH] =
        getNodeLayout(menuBackgroundNode, menuNodeX, menuNodeY);

    const auto [arX, arY, arW, arH] =
        getNodeLayout(aspectRatioNode, menuBgX, menuBgY);
    const auto [resX, resY, resW, resH] =
        getNodeLayout(resolutionNode, menuBgX, menuBgY);
    const auto [fsX, fsY, fsW, fsH] =
        getNodeLayout(fullScreenNode, menuBgX, menuBgY);
    const auto [vsX, vsY, vsW, vsH] =
        getNodeLayout(verticalSyncNode, menuBgX, menuBgY);
    const auto [aaX, aaY, aaW, aaH] =
        getNodeLayout(antiAliasingNode, menuBgX, menuBgY);
    const auto [sqX, sqY, sqW, sqH] =
        getNodeLayout(shadowQualityNode, menuBgX, menuBgY);
    const auto [beX, beY, beW, beH] =
        getNodeLayout(bloomEnabledNode, menuBgX, menuBgY);
    const auto [btX, btY, btW, btH] =
        getNodeLayout(bloomThresholdNode, menuBgX, menuBgY);
    const auto [biX, biY, biW, biH] =
        getNodeLayout(bloomIntensityNode, menuBgX, menuBgY);
    const auto [retX, retY, retW, retH] =
        getNodeLayout(returnNode, menuBgX, menuBgY);

    auto renderDots = [&](const size_t count, const float valueCenterX,
                          const float rowY, const float rowH,
                          const size_t                 displayIdx,
                          const std::optional<size_t>& currentIdx) {
        if (count == 0) {
            return;
        }
        const float dotRadius =
            std::max(3.F, std::round(static_cast<float>(fontSize) / 12.F));
        const float dotSpacing = dotRadius * 3.5F;
        const float totalW =
            (static_cast<float>(count) - 1.F) * dotSpacing + dotRadius * 2.F;
        float       dotX = valueCenterX - totalW / 2.F;
        const float dotY = rowY + rowH - dotRadius * 2.F - 9.F;
        for (size_t i = 0; i < count; i++) {
            const bool isCurrent = currentIdx.has_value() && i == *currentIdx;
            const bool isDisplay = i == displayIdx;
            const auto color     = isCurrent ? dotColorCurrent :
                                   isDisplay ? dotColorDisplay :
                                               dotColorDefault;
            quad->render({ dotX, dotY },
                         { dotX + dotRadius * 2.F, dotY + dotRadius * 2.F },
                         color, dotRadius);
            dotX += dotSpacing;
        }
    };

    renderRowBackground(arX, arY, arW, arH, OptionMenuItem::AspectRatio);
    aspectRatioList->onUpdate(arX, arY, arW, arH, "Aspect Ratio");
    renderDots(validAspectRatioFilters.size(),
               aspectRatioList->getValueCenterX(arX, arW), arY, arH,
               aspectRatioList->getSelectedIndex(), currentAspectRatioIndex);

    renderRowBackground(resX, resY, resW, resH, OptionMenuItem::Resolution);
    resolutionList->onUpdate(resX, resY, resW, resH, "Resolution");
    renderDots(filteredResolutions.size(),
               resolutionList->getValueCenterX(resX, resW), resY, resH,
               resolutionList->getSelectedIndex(), currentResolutionIndex);

    auto renderRowLabel = [&](const float x, const float y, const float h,
                              const std::string_view label) {
        const float textY = std::floor(
            y + (h - static_cast<float>(menuFont->getHeight(fontSize))) / 2.F);
        menuFont->beginPass(fontSize);
        menuFont->render(label, { x + textMarginLeft, textY }, textColor);
        menuFont->endPass();
    };

    renderRowBackground(fsX, fsY, fsW, fsH, OptionMenuItem::FullScreen);
    renderRowLabel(fsX, fsY, fsH, "Full Screen");
    fullScreenCheckbox->onUpdate(fsX, fsY, fsW, fsH, pendingFullscreen);

    renderRowBackground(vsX, vsY, vsW, vsH, OptionMenuItem::VerticalSync);
    renderRowLabel(vsX, vsY, vsH, "Vertical Sync");
    verticalSyncCheckbox->onUpdate(vsX, vsY, vsW, vsH, pendingVsync);

    renderRowBackground(aaX, aaY, aaW, aaH, OptionMenuItem::AntiAliasing);
    renderRowLabel(aaX, aaY, aaH, "Anti-Aliasing");
    antiAliasingCheckbox->onUpdate(aaX, aaY, aaW, aaH, pendingFxaa);

    renderRowBackground(sqX, sqY, sqW, sqH, OptionMenuItem::ShadowQuality);
    shadowQualityList->onUpdate(sqX, sqY, sqW, sqH, "Shadow Map");
    renderDots(shadowResolutions.size(),
               shadowQualityList->getValueCenterX(sqX, sqW), sqY, sqH,
               shadowQualityList->getSelectedIndex(), currentShadowResIndex);

    renderRowBackground(beX, beY, beW, beH, OptionMenuItem::BloomEnabled);
    renderRowLabel(beX, beY, beH, "Bloom");
    bloomEnabledCheckbox->onUpdate(beX, beY, beW, beH, pendingBloomEnabled);

    renderRowBackground(btX, btY, btW, btH, OptionMenuItem::BloomThreshold);
    bloomThresholdList->onUpdate(btX, btY, btW, btH, "Bloom Threshold");
    renderDots(bloomThresholds.size(),
               bloomThresholdList->getValueCenterX(btX, btW), btY, btH,
               static_cast<size_t>(pendingBloomThreshold), std::nullopt);

    renderRowBackground(biX, biY, biW, biH, OptionMenuItem::BloomIntensity);
    bloomIntensityList->onUpdate(biX, biY, biW, biH, "Bloom Intensity");
    renderDots(bloomIntensities.size(),
               bloomIntensityList->getValueCenterX(biX, biW), biY, biH,
               static_cast<size_t>(pendingBloomIntensity), std::nullopt);

    returnButton->setPosition({ retX, retY }, { retX + retW, retY + retH });
    if (selectedItem == OptionMenuItem::Return) {
        returnButton->setBorderWidth(selectedBorderWidth);
        returnButton->setBorderColor(glm::vec4{ 1.F });
        returnButton->setButtonColor(textHoverColor);
    } else if (!returnButton->hasHover()) {
        returnButton->setBorderWidth(0.F);
        returnButton->setButtonColor(glm::vec4{ 0.F });
    } else {
        returnButton->setBorderWidth(0.F);
        returnButton->setButtonColor(hoverColor);
    }

    UNUSED(returnButton->onUpdate(elapsedTime));

    if (!isActive()) {
        wasActiveLastFrame    = false;
        waitForConfirmRelease = false;
    }

    return true;
}

void OptionLayer::renderRowBackground(float x, float y, const float w,
                                      const float          h,
                                      const OptionMenuItem item) const {
    const bool isSelected = selectedItem == item;
    const bool isHovered  = hoveredItem == item;
    quad->render({ x, y }, { x + w, y + h },
                 isSelected ? textHoverColor :
                 isHovered  ? hoverColor :
                              buttonColor,
                 cornerRadius, isSelected ? selectedBorderWidth : 0.F,
                 glm::vec4{ 1.F });
}

void OptionLayer::recalculateLayout(const float width, const float height) {
    YGNodeStyleSetWidth(rootNode, width);
    YGNodeStyleSetHeight(rootNode, height);
    YGNodeCalculateLayout(rootNode, width, height, YGDirectionLTR);
}

bool OptionLayer::onMouseButtonPressed(const MouseButtonPressedEvent& event) {
    if (event.getMouseButton() != sponge::input::MouseButton::Button0) {
        return false;
    }

    auto [mouseX, mouseY] =
        sponge::platform::glfw::core::Application::get().getMousePosition();

    if (returnButton->isInside({ mouseX, mouseY })) {
        if (hasUnappliedChanges) {
            applyChanges();
        } else {
            clearHoveredItems();
            resetSelectionToCurrentState();
            setActive(false);
        }
        return true;
    }

    auto [rootNodeX, rootNodeY, rootNodeW, rootNodeH] =
        getNodeLayout(rootNode, 0.F, 0.F);
    auto [menuNodeX, menuNodeY, menuNodeW, menuNodeH] =
        getNodeLayout(menuNode, rootNodeX, rootNodeY);
    auto [menuBackgroundNodeX, menuBackgroundNodeY, menuBackgroundNodeW,
          menuBackgroundNodeH] =
        getNodeLayout(menuBackgroundNode, menuNodeX, menuNodeY);

    const auto [arX, arY, arW, arH] = getNodeLayout(
        aspectRatioNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= arX && mouseX <= arX + arW && mouseY >= arY &&
        mouseY <= arY + arH) {
        selectedItem = OptionMenuItem::AspectRatio;

        if (aspectRatioList->isInsideLeft(mouseX, arX, arW)) {
            aspectRatioList->selectPrev();
            filterResolutions();
        } else if (aspectRatioList->isInsideRight(mouseX, arX, arW)) {
            aspectRatioList->selectNext();
            filterResolutions();
        }

        return true;
    }

    const auto [resX, resY, resW, resH] =
        getNodeLayout(resolutionNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= resX && mouseX <= resX + resW && mouseY >= resY &&
        mouseY <= resY + resH) {
        selectedItem = OptionMenuItem::Resolution;

        if (!filteredResolutions.empty()) {
            if (resolutionList->isInsideLeft(mouseX, resX, resW)) {
                resolutionList->selectPrev();
                updateChangeStatus();
            } else if (resolutionList->isInsideRight(mouseX, resX, resW)) {
                resolutionList->selectNext();
                updateChangeStatus();
            }
        }

        return true;
    }

    const auto [fullX, fullY, fullW, fullH] =
        getNodeLayout(fullScreenNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= fullX && mouseX <= fullX + fullW && mouseY >= fullY &&
        mouseY <= fullY + fullH) {
        selectedItem = OptionMenuItem::FullScreen;
        if (fullScreenCheckbox->isInside(mouseX, mouseY, fullX, fullY, fullW,
                                         fullH)) {
            pendingFullscreen = !pendingFullscreen;
            updateChangeStatus();
        }
        return true;
    }

    const auto [vsyncX, vsyncY, vsyncW, vsyncH] = getNodeLayout(
        verticalSyncNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= vsyncX && mouseX <= vsyncX + vsyncW && mouseY >= vsyncY &&
        mouseY <= vsyncY + vsyncH) {
        selectedItem = OptionMenuItem::VerticalSync;
        if (verticalSyncCheckbox->isInside(mouseX, mouseY, vsyncX, vsyncY,
                                           vsyncW, vsyncH)) {
            pendingVsync = !pendingVsync;
            updateChangeStatus();
        }
        return true;
    }

    const auto [aaX, aaY, aaW, aaH] = getNodeLayout(
        antiAliasingNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= aaX && mouseX <= aaX + aaW && mouseY >= aaY &&
        mouseY <= aaY + aaH) {
        selectedItem = OptionMenuItem::AntiAliasing;
        if (antiAliasingCheckbox->isInside(mouseX, mouseY, aaX, aaY, aaW,
                                           aaH)) {
            pendingFxaa = !pendingFxaa;
            updateChangeStatus();
        }
        return true;
    }

    const auto [sqX, sqY, sqW, sqH] = getNodeLayout(
        shadowQualityNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= sqX && mouseX <= sqX + sqW && mouseY >= sqY &&
        mouseY <= sqY + sqH) {
        selectedItem = OptionMenuItem::ShadowQuality;
        if (shadowQualityList->isInsideLeft(mouseX, sqX, sqW)) {
            shadowQualityList->selectPrev();
            pendingShadowResIndex =
                static_cast<int>(shadowQualityList->getSelectedIndex());
            updateChangeStatus();
        } else if (shadowQualityList->isInsideRight(mouseX, sqX, sqW)) {
            shadowQualityList->selectNext();
            pendingShadowResIndex =
                static_cast<int>(shadowQualityList->getSelectedIndex());
            updateChangeStatus();
        }
        return true;
    }

    const auto [beX2, beY2, beW2, beH2] = getNodeLayout(
        bloomEnabledNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= beX2 && mouseX <= beX2 + beW2 && mouseY >= beY2 &&
        mouseY <= beY2 + beH2) {
        selectedItem = OptionMenuItem::BloomEnabled;
        if (bloomEnabledCheckbox->isInside(mouseX, mouseY, beX2, beY2, beW2,
                                           beH2)) {
            pendingBloomEnabled = !pendingBloomEnabled;
            updateChangeStatus();
        }
        return true;
    }

    const auto [btX2, btY2, btW2, btH2] = getNodeLayout(
        bloomThresholdNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= btX2 && mouseX <= btX2 + btW2 && mouseY >= btY2 &&
        mouseY <= btY2 + btH2) {
        selectedItem = OptionMenuItem::BloomThreshold;
        if (bloomThresholdList->isInsideLeft(mouseX, btX2, btW2)) {
            bloomThresholdList->selectPrev();
            pendingBloomThreshold =
                static_cast<int>(bloomThresholdList->getSelectedIndex());
            updateChangeStatus();
        } else if (bloomThresholdList->isInsideRight(mouseX, btX2, btW2)) {
            bloomThresholdList->selectNext();
            pendingBloomThreshold =
                static_cast<int>(bloomThresholdList->getSelectedIndex());
            updateChangeStatus();
        }
        return true;
    }

    const auto [biX2, biY2, biW2, biH2] = getNodeLayout(
        bloomIntensityNode, menuBackgroundNodeX, menuBackgroundNodeY);

    if (mouseX >= biX2 && mouseX <= biX2 + biW2 && mouseY >= biY2 &&
        mouseY <= biY2 + biH2) {
        selectedItem = OptionMenuItem::BloomIntensity;
        if (bloomIntensityList->isInsideLeft(mouseX, biX2, biW2)) {
            bloomIntensityList->selectPrev();
            pendingBloomIntensity =
                static_cast<int>(bloomIntensityList->getSelectedIndex());
            updateChangeStatus();
        } else if (bloomIntensityList->isInsideRight(mouseX, biX2, biW2)) {
            bloomIntensityList->selectNext();
            pendingBloomIntensity =
                static_cast<int>(bloomIntensityList->getSelectedIndex());
            updateChangeStatus();
        }
        return true;
    }

    return true;
}

bool OptionLayer::onMouseMoved(const MouseMovedEvent& event) {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    auto updateHover = [&pos](ui::Button* button) {
        if (!button->hasHover() && button->isInside(pos)) {
            button->setHover(true);
        } else if (button->hasHover() && !button->isInside(pos)) {
            button->setHover(false);
        }
    };

    updateHover(returnButton.get());

    const auto rootNodeLayout = getNodeLayout(rootNode, 0.F, 0.F);
    const auto menuNodeLayout = getNodeLayout(
        menuNode, std::get<0>(rootNodeLayout), std::get<1>(rootNodeLayout));
    const auto menuBackgroundNodeLayout =
        getNodeLayout(menuBackgroundNode, std::get<0>(menuNodeLayout),
                      std::get<1>(menuNodeLayout));

    const float menuBackgroundNodeX = std::get<0>(menuBackgroundNodeLayout);
    const float menuBackgroundNodeY = std::get<1>(menuBackgroundNodeLayout);

    auto isOver = [&](auto* node) {
        const auto nodeLayout =
            getNodeLayout(node, menuBackgroundNodeX, menuBackgroundNodeY);
        const float x = std::get<0>(nodeLayout);
        const float y = std::get<1>(nodeLayout);
        const float w = std::get<2>(nodeLayout);
        const float h = std::get<3>(nodeLayout);
        return pos.x >= x && pos.x <= x + w && pos.y >= y && pos.y <= y + h;
    };

    if (isOver(aspectRatioNode)) {
        hoveredItem = OptionMenuItem::AspectRatio;
    } else if (isOver(resolutionNode)) {
        hoveredItem = OptionMenuItem::Resolution;
    } else if (isOver(fullScreenNode)) {
        hoveredItem = OptionMenuItem::FullScreen;
    } else if (isOver(verticalSyncNode)) {
        hoveredItem = OptionMenuItem::VerticalSync;
    } else if (isOver(antiAliasingNode)) {
        hoveredItem = OptionMenuItem::AntiAliasing;
    } else if (isOver(shadowQualityNode)) {
        hoveredItem = OptionMenuItem::ShadowQuality;
    } else if (isOver(bloomEnabledNode)) {
        hoveredItem = OptionMenuItem::BloomEnabled;
    } else if (isOver(bloomThresholdNode)) {
        hoveredItem = OptionMenuItem::BloomThreshold;
    } else if (isOver(bloomIntensityNode)) {
        hoveredItem = OptionMenuItem::BloomIntensity;
    } else {
        hoveredItem = std::nullopt;
    }

    return true;
}

bool OptionLayer::onWindowResize(const WindowResizeEvent& event) {
    orthoCamera->setWidthAndHeight(event.getWidth(), event.getHeight());

    const auto width  = static_cast<float>(event.getWidth());
    const auto height = static_cast<float>(event.getHeight());
    recalculateLayout(width, height);

    const auto newFontSize = ui::menuFontSizeForWidth(event.getWidth());
    if (newFontSize != fontSize) {
        fontSize = newFontSize;

        returnButton->setFontSize(fontSize);
        aspectRatioList->setFontSize(fontSize);
        resolutionList->setFontSize(fontSize);
        shadowQualityList->setFontSize(fontSize);
        bloomThresholdList->setFontSize(fontSize);
        bloomIntensityList->setFontSize(fontSize);

        const auto checkboxSize = static_cast<float>(fontSize);
        antiAliasingCheckbox->setSize(checkboxSize);
        fullScreenCheckbox->setSize(checkboxSize);
        verticalSyncCheckbox->setSize(checkboxSize);
        bloomEnabledCheckbox->setSize(checkboxSize);

        const float maxCycleValueWidth = computeMaxCycleValueWidth();

        aspectRatioList->setMaxValueWidth(maxCycleValueWidth);
        resolutionList->setMaxValueWidth(maxCycleValueWidth);
        shadowQualityList->setMaxValueWidth(maxCycleValueWidth);
        bloomThresholdList->setMaxValueWidth(maxCycleValueWidth);
        bloomIntensityList->setMaxValueWidth(maxCycleValueWidth);
    }

    if (isActive()) {
        updateChangeStatus();
    }

    return false;
}

void OptionLayer::filterResolutions() {
    if (validAspectRatioFilters.empty()) {
        filteredResolutions.clear();
        resolutionList->setItems({});
        currentResolutionIndex = std::nullopt;
        updateChangeStatus();
        return;
    }

    const auto& filter =
        validAspectRatioFilters[aspectRatioList->getSelectedIndex()];

    auto matchesFilter = [&](const auto& res) {
        return matchesAspectRatio(filter, res.width, res.height);
    };

    auto filtered = availableResolutions | std::views::filter(matchesFilter);
    filteredResolutions =
        std::vector<sponge::core::Resolution>(filtered.begin(), filtered.end());

    const auto window        = Maze::get().getWindow();
    const auto currentWidth  = window->getWidth();
    const auto currentHeight = window->getHeight();

    std::vector<std::string> resItems;
    resItems.reserve(filteredResolutions.size());
    for (const auto& res : filteredResolutions) {
        resItems.emplace_back(fmt::format("{} × {}", res.width, res.height));
    }
    resolutionList->setItems(std::move(resItems));

    if (!filteredResolutions.empty()) {
        const auto it = std::find_if(
            filteredResolutions.begin(), filteredResolutions.end(),
            [&](const auto& r) {
                return r.width == currentWidth && r.height == currentHeight;
            });
        const auto idx =
            it != filteredResolutions.end() ?
                std::optional<size_t>{ static_cast<size_t>(
                    std::distance(filteredResolutions.begin(), it)) } :
                std::nullopt;
        resolutionList->setSelectedIndex(idx.value_or(0));
        currentResolutionIndex = idx;
    } else {
        currentResolutionIndex = std::nullopt;
    }

    updateChangeStatus();
}

void OptionLayer::applyChanges() {
    const auto currentWindow = Maze::get().getWindow();
    auto       saveWidth     = currentWindow->getWidth();
    auto       saveHeight    = currentWindow->getHeight();

    if (!filteredResolutions.empty()) {
        const auto& res =
            filteredResolutions[resolutionList->getSelectedIndex()];
        saveWidth  = res.width;
        saveHeight = res.height;
        Maze::get().setResolution(res.width, res.height);
    }
    if (pendingFullscreen != Maze::get().isFullscreen()) {
        Maze::get().toggleFullscreen();
    }
    Maze::get().setVerticalSync(pendingVsync);
    Maze::get().setFxaaEnabled(pendingFxaa);
    Maze::get().setBloomEnabled(pendingBloomEnabled);
    Maze::get().setBloomThreshold(
        bloomThresholds[static_cast<size_t>(pendingBloomThreshold)]);
    Maze::get().setBloomIntensity(
        bloomIntensities[static_cast<size_t>(pendingBloomIntensity)]);
    const auto shadowRes =
        shadowResolutions[static_cast<size_t>(pendingShadowResIndex)];
    Maze::get().getMazeLayer()->setShadowMapRes(shadowRes);
    saveVideoSettings(
        saveWidth, saveHeight, pendingFullscreen, pendingVsync, pendingFxaa,
        shadowRes, pendingBloomEnabled,
        bloomThresholds[static_cast<size_t>(pendingBloomThreshold)],
        bloomIntensities[static_cast<size_t>(pendingBloomIntensity)]);

    syncPendingCheckboxState();
    hasUnappliedChanges = false;
    returnButton->setMessage(returnMessage);
}

void OptionLayer::syncPendingCheckboxState() {
    pendingFullscreen   = Maze::get().isFullscreen();
    pendingVsync        = Maze::get().hasVerticalSync();
    pendingFxaa         = Maze::get().isFxaaEnabled();
    pendingBloomEnabled = Maze::get().isBloomEnabled();

    const float curThreshold = Maze::get().getBloomThreshold();
    const auto  tIt =
        std::ranges::find_if(bloomThresholds, [curThreshold](float v) {
            return std::abs(v - curThreshold) < 0.01F;
        });
    pendingBloomThreshold =
        tIt != bloomThresholds.end() ?
            static_cast<int>(tIt - bloomThresholds.begin()) :
            1;
    if (bloomThresholdList) {
        bloomThresholdList->setSelectedIndex(
            static_cast<size_t>(pendingBloomThreshold));
    }

    const float curIntensity = Maze::get().getBloomIntensity();
    const auto  iIt =
        std::ranges::find_if(bloomIntensities, [curIntensity](float v) {
            return std::abs(v - curIntensity) < 0.01F;
        });
    pendingBloomIntensity =
        iIt != bloomIntensities.end() ?
            static_cast<int>(iIt - bloomIntensities.begin()) :
            1;
    if (bloomIntensityList) {
        bloomIntensityList->setSelectedIndex(
            static_cast<size_t>(pendingBloomIntensity));
    }

    const auto currentShadowRes =
        Maze::get().getMazeLayer()->getDirectionalLightShadowMapRes();
    const auto it = std::ranges::find(shadowResolutions, currentShadowRes);
    pendingShadowResIndex =
        it != shadowResolutions.end() ?
            static_cast<int>(it - shadowResolutions.begin()) :
            1;
    if (shadowQualityList) {
        shadowQualityList->setSelectedIndex(
            static_cast<size_t>(pendingShadowResIndex));
    }
}

void OptionLayer::updateChangeStatus() {
    const bool checkboxChanged =
        pendingFullscreen != Maze::get().isFullscreen() ||
        pendingVsync != Maze::get().hasVerticalSync() ||
        pendingFxaa != Maze::get().isFxaaEnabled() ||
        pendingBloomEnabled != Maze::get().isBloomEnabled();

    const auto window = Maze::get().getWindow();
    const auto curW   = window->getWidth();
    const auto curH   = window->getHeight();

    if (!validAspectRatioFilters.empty()) {
        const auto idx = findAspectRatioIndex(curW, curH);
        currentAspectRatioIndex =
            matchesAspectRatio(validAspectRatioFilters[idx], curW, curH) ?
                std::optional<size_t>{ idx } :
                std::nullopt;
    } else {
        currentAspectRatioIndex = std::nullopt;
    }

    bool resolutionChanged = false;
    if (!filteredResolutions.empty()) {
        const auto& res =
            filteredResolutions[resolutionList->getSelectedIndex()];
        resolutionChanged = res.width != curW || res.height != curH;

        const auto it = std::find_if(
            filteredResolutions.begin(), filteredResolutions.end(),
            [&](const auto& r) { return r.width == curW && r.height == curH; });
        currentResolutionIndex =
            it != filteredResolutions.end() ?
                std::optional<size_t>{ static_cast<size_t>(
                    std::distance(filteredResolutions.begin(), it)) } :
                std::nullopt;
    }

    const auto currentShadowRes =
        Maze::get().getMazeLayer()->getDirectionalLightShadowMapRes();
    const auto sqIt = std::ranges::find(shadowResolutions, currentShadowRes);
    currentShadowResIndex = sqIt != shadowResolutions.end() ?
                                std::optional<size_t>{ static_cast<size_t>(
                                    sqIt - shadowResolutions.begin()) } :
                                std::nullopt;
    const bool shadowChanged =
        shadowResolutions[static_cast<size_t>(pendingShadowResIndex)] !=
        currentShadowRes;

    const bool bloomThresholdChanged =
        bloomThresholds[static_cast<size_t>(pendingBloomThreshold)] !=
        Maze::get().getBloomThreshold();
    const bool bloomIntensityChanged =
        bloomIntensities[static_cast<size_t>(pendingBloomIntensity)] !=
        Maze::get().getBloomIntensity();
    hasUnappliedChanges = checkboxChanged || resolutionChanged ||
                          shadowChanged || bloomThresholdChanged ||
                          bloomIntensityChanged;
    returnButton->setMessage(hasUnappliedChanges ? applyMessage :
                                                   returnMessage);
}

void OptionLayer::clearHoveredItems() {
    returnButton->setHover(false);
    hoveredItem = std::nullopt;
}

void OptionLayer::resetSelectionToCurrentState() {
    syncPendingCheckboxState();

    const auto window        = Maze::get().getWindow();
    const auto currentWidth  = window->getWidth();
    const auto currentHeight = window->getHeight();

    aspectRatioList->setSelectedIndex(
        findAspectRatioIndex(currentWidth, currentHeight));

    filterResolutions();
}
}  // namespace game::layer
