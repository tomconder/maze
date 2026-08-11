#include "layer/optionlayer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <yoga/Yoga.h>

#include "core/base.hpp"
#include "core/settings.hpp"
#include "core/window.hpp"
#include "event/event.hpp"
#include "input/gameaction.hpp"
#include "input/inputcontext.hpp"
#include "input/mousecode.hpp"
#include "maze.hpp"
#include "platform/glfw/core/application.hpp"
#include "platform/opengl/renderer/assetmanager.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "resourcemanager.hpp"
#include "scene/orthocamera.hpp"
#include "ui/button.hpp"
#include "ui/checkbox.hpp"
#include "ui/menufontsize.hpp"
#include "ui/menulayout.hpp"
#include "ui/menuselection.hpp"
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
    {
        .label       = "~16:9",
        .numerator   = 16,
        .denominator = 9,
        .approximate = true,
    },
    { .label = "16:10", .numerator = 16, .denominator = 10 },
    { .label = "25:16", .numerator = 25, .denominator = 16 },
});

constexpr std::string_view returnMessage = "Return";
constexpr std::string_view applyMessage  = "Apply";

constexpr std::array<uint32_t, 4> shadowResolutions = { 512, 1024, 2048, 4096 };

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
    return std::ranges::any_of(resolutions, [&](const auto& res) {
        return matchesAspectRatio(filter, res.width, res.height);
    });
}

size_t findAspectRatioIndex(const uint32_t width, const uint32_t height) {
    const auto g  = std::gcd(width, height);
    const auto rw = width / g;
    const auto rh = height / g;
    auto it = std::ranges::find_if(validAspectRatioFilters, [&](const auto& f) {
        const auto fg = std::gcd(f.numerator, f.denominator);
        return !f.approximate && rw == f.numerator / fg &&
               rh == f.denominator / fg;
    });
    if (it != validAspectRatioFilters.end()) {
        return static_cast<size_t>(
            std::distance(validAspectRatioFilters.begin(), it));
    }
    it = std::ranges::find_if(validAspectRatioFilters, [&](const auto& f) {
        return matchesAspectRatio(f, width, height);
    });
    return it != validAspectRatioFilters.end() ?
               static_cast<size_t>(
                   std::distance(validAspectRatioFilters.begin(), it)) :
               0;
}

std::shared_ptr<sponge::platform::opengl::scene::BitmapFont> menuFont;

// max display width across all aspect-ratio labels and resolution strings.
// Measured with tabularFigures=true to match how they render.
float computeMaxCycleValueWidth() {
    float width = std::accumulate(
        validAspectRatioFilters.begin(), validAspectRatioFilters.end(), 0.F,
        [](const float acc, const auto& f) {
            return std::max(acc, static_cast<float>(menuFont->getLength(
                                     f.label, fontSize, true)));
        });
    return std::accumulate(
        availableResolutions.begin(), availableResolutions.end(), width,
        [](const float acc, const auto& res) {
            return std::max(acc,
                            static_cast<float>(menuFont->getLength(
                                fmt::format("{} × {}", res.width, res.height),
                                fontSize, true)));
        });
}

void saveVideoSettings(const uint32_t width, const uint32_t height,
                       const bool fullscreen, const bool vsync, const bool fxaa,
                       const uint32_t shadowRes) {
    using sponge::core::Settings;
    Settings::set("video.width", width);
    Settings::set("video.height", height);
    Settings::set("video.fullscreen", fullscreen);
    Settings::set("video.vsync", vsync);
    Settings::set("video.fxaa", fxaa);
    Settings::set("video.shadowRes", shadowRes);
    Settings::save();
}

std::unique_ptr<game::ui::Button> returnButton;

YGNodeRef menuBackgroundNode = nullptr;
YGNodeRef menuNode           = nullptr;
YGNodeRef rootNode           = nullptr;

using game::layer::OptionMenuItem;

constexpr size_t rowCount = static_cast<size_t>(OptionMenuItem::Count);

// Rows are laid out top-to-bottom in OptionMenuItem order, so the enum
// doubles as the row index.
std::array<YGNodeRef, rowCount> rowNodes{};

struct RowDef {
    OptionMenuItem   item;
    std::string_view label;
};

// Every row except Return, which is a Button rather than a setting widget.
constexpr std::array settingRows = {
    RowDef{ .item = OptionMenuItem::AspectRatio, .label = "Aspect Ratio" },
    RowDef{ .item = OptionMenuItem::Resolution, .label = "Resolution" },
    RowDef{ .item = OptionMenuItem::FullScreen, .label = "Full Screen" },
    RowDef{ .item = OptionMenuItem::VerticalSync, .label = "Vertical Sync" },
    RowDef{ .item = OptionMenuItem::AntiAliasing, .label = "Anti-Aliasing" },
    RowDef{ .item = OptionMenuItem::ShadowQuality, .label = "Shadow Map" },
};

// rowNodes are created by index, so the table must stay in enum order and
// cover every row but Return.
static_assert(settingRows.size() + 1 == rowCount);
static_assert([] {
    for (size_t i = 0; i < settingRows.size(); i++) {
        if (static_cast<size_t>(settingRows[i].item) != i) {
            return false;
        }
    }
    return true;
}());

std::optional<size_t> currentShadowResIndex;

// Dot strip state for the three cycling rows: total items, applied index.
std::pair<size_t, std::optional<size_t>> dotState(const OptionMenuItem item) {
    switch (item) {
        case OptionMenuItem::AspectRatio:
            return { validAspectRatioFilters.size(), currentAspectRatioIndex };
        case OptionMenuItem::Resolution:
            return { filteredResolutions.size(), currentResolutionIndex };
        case OptionMenuItem::ShadowQuality:
            return { shadowResolutions.size(), currentShadowResIndex };
        default:
            return { 0, std::nullopt };
    }
}

bool contains(const float x, const float y, const float w, const float h,
              const float px, const float py) {
    return px >= x && px <= x + w && py >= y && py <= y + h;
}

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
    const auto fontCreateInfo = FontCreateInfo{
        .name = std::string(fontName),
        .path = std::string(fontPath),
    };
    menuFont = AssetManager::createFont(fontCreateInfo);

    const auto orthoCameraCreateInfo =
        scene::OrthoCameraCreateInfo{ .name = std::string(cameraName) };
    orthoCamera = ResourceManager::createOrthoCamera(orthoCameraCreateInfo);

    quad = std::make_unique<Quad>();

    fontSize = ui::menuFontSizeForWidth(orthoCamera->getWidth());

    returnButton = ui::makeMenuButton(returnMessage, fontSize, menuFont,
                                      buttonColor, textColor);

    for (const auto& shader : { menuFont->getShader(), Quad::getShader() }) {
        shader->bind();
        shader->setMat4("projection", orthoCamera->getProjection());
        shader->unbind();
    }

    const auto skeleton = ui::buildMenuSkeleton(45.F);
    rootNode            = skeleton.root;
    menuNode            = skeleton.menu;
    menuBackgroundNode  = skeleton.menuBackground;

    for (size_t i = 0; i < rowNodes.size(); i++) {
        rowNodes[i] = ui::makeMenuRow(menuBackgroundNode, static_cast<int>(i));
    }

    availableResolutions =
        sponge::platform::glfw::core::Application::getAvailableResolutions();

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
    // tabular figures: aspect ratio/resolution digits shouldn't shift width
    // as the player cycles through values
    auto numericCreateInfo           = selectCreateInfo;
    numericCreateInfo.tabularFigures = true;
    aspectRatioList   = std::make_unique<ui::SelectList>(numericCreateInfo);
    resolutionList    = std::make_unique<ui::SelectList>(numericCreateInfo);
    shadowQualityList = std::make_unique<ui::SelectList>(selectCreateInfo);

    const ui::CheckboxCreateInfo checkboxCreateInfo{
        .margin = textMarginLeft,
        .size   = static_cast<float>(fontSize),
    };
    antiAliasingCheckbox = std::make_unique<ui::Checkbox>(checkboxCreateInfo);
    fullScreenCheckbox   = std::make_unique<ui::Checkbox>(checkboxCreateInfo);
    verticalSyncCheckbox = std::make_unique<ui::Checkbox>(checkboxCreateInfo);

    std::vector<std::string> arItems;
    arItems.reserve(validAspectRatioFilters.size());
    for (const auto& f : validAspectRatioFilters) {
        arItems.emplace_back(f.label);
    }
    aspectRatioList->setItems(std::move(arItems));

    {
        shadowQualityList->setItems({ "Low", "Normal", "High", "Ultra" });
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
                resetSelectionToCurrentState();
                waitForConfirmRelease = input.isHeld(GameAction::MenuConfirm);
            } else if (waitForConfirmRelease &&
                       !input.isHeld(GameAction::MenuConfirm)) {
                waitForConfirmRelease = false;
            }
        }

        if (wasActiveLastFrame) {
            const auto& input = mgr.getSnapshot();

            ui::stepSelection(input, selectedItem);

            if (input.isActive(GameAction::MenuLeft)) {
                cycleList(selectedItem, -1);
            }
            if (input.isActive(GameAction::MenuRight)) {
                cycleList(selectedItem, 1);
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
                } else {
                    togglePending(selectedItem);
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

    auto renderRowLabel = [&](const float x, const float y, const float h,
                              const std::string_view label) {
        const float textY = std::floor(
            y + (h - static_cast<float>(menuFont->getHeight(fontSize))) / 2.F);
        menuFont->beginPass(fontSize);
        menuFont->render(label, { x + textMarginLeft, textY }, textColor);
        menuFont->endPass();
    };

    for (const auto& [item, label] : settingRows) {
        const auto [x, y, w, h] = rowLayout(item);
        renderRowBackground(x, y, w, h, item);

        if (auto* const list = listFor(item); list != nullptr) {
            list->onUpdate(x, y, w, h, label);
            const auto [dotCount, appliedIndex] = dotState(item);
            renderDots(dotCount, list->getValueCenterX(x, w), y, h,
                       list->getSelectedIndex(), appliedIndex);
        } else {
            renderRowLabel(x, y, h, label);
            checkboxFor(item)->onUpdate(x, y, w, h, *pendingFor(item));
        }
    }

    const auto [retX, retY, retW, retH] = rowLayout(OptionMenuItem::Return);
    returnButton->setPosition({ retX, retY }, { retX + retW, retY + retH });
    ui::updateMenuButtonVisuals(returnButton.get(),
                                selectedItem == OptionMenuItem::Return,
                                textHoverColor);

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

std::tuple<float, float, float, float>
    OptionLayer::rowLayout(const OptionMenuItem item) {
    const auto [rootX, rootY, rootW, rootH] =
        ui::getNodeLayout(rootNode, 0.F, 0.F);
    const auto [menuX, menuY, menuW, menuH] =
        ui::getNodeLayout(menuNode, rootX, rootY);
    const auto [bgX, bgY, bgW, bgH] =
        ui::getNodeLayout(menuBackgroundNode, menuX, menuY);
    return ui::getNodeLayout(rowNodes[static_cast<size_t>(item)], bgX, bgY);
}

ui::SelectList* OptionLayer::listFor(const OptionMenuItem item) const {
    switch (item) {
        case OptionMenuItem::AspectRatio:
            return aspectRatioList.get();
        case OptionMenuItem::Resolution:
            return resolutionList.get();
        case OptionMenuItem::ShadowQuality:
            return shadowQualityList.get();
        default:
            return nullptr;
    }
}

ui::Checkbox* OptionLayer::checkboxFor(const OptionMenuItem item) const {
    switch (item) {
        case OptionMenuItem::FullScreen:
            return fullScreenCheckbox.get();
        case OptionMenuItem::VerticalSync:
            return verticalSyncCheckbox.get();
        case OptionMenuItem::AntiAliasing:
            return antiAliasingCheckbox.get();
        default:
            return nullptr;
    }
}

bool* OptionLayer::pendingFor(const OptionMenuItem item) {
    switch (item) {
        case OptionMenuItem::FullScreen:
            return &pendingFullscreen;
        case OptionMenuItem::VerticalSync:
            return &pendingVsync;
        case OptionMenuItem::AntiAliasing:
            return &pendingFxaa;
        default:
            return nullptr;
    }
}

// delta < 0 steps back, otherwise forward. No-op on non-cycling rows.
void OptionLayer::cycleList(const OptionMenuItem item, const int delta) {
    auto* const list = listFor(item);
    if (list == nullptr ||
        (item == OptionMenuItem::Resolution && filteredResolutions.empty())) {
        return;
    }

    if (delta < 0) {
        list->selectPrev();
    } else {
        list->selectNext();
    }

    if (item == OptionMenuItem::AspectRatio) {
        // filterResolutions() ends in updateChangeStatus()
        filterResolutions();
        return;
    }

    if (item == OptionMenuItem::ShadowQuality) {
        pendingShadowResIndex = static_cast<int>(list->getSelectedIndex());
    }
    updateChangeStatus();
}

void OptionLayer::togglePending(const OptionMenuItem item) {
    auto* const pending = pendingFor(item);
    if (pending == nullptr) {
        return;
    }
    *pending = !*pending;
    updateChangeStatus();
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

    for (const auto& [item, label] : settingRows) {
        const auto [x, y, w, h] = rowLayout(item);
        if (!contains(x, y, w, h, mouseX, mouseY)) {
            continue;
        }

        selectedItem = item;

        if (auto* const list = listFor(item); list != nullptr) {
            if (list->isInsideLeft(mouseX, x, w)) {
                cycleList(item, -1);
            } else if (list->isInsideRight(mouseX, x, w)) {
                cycleList(item, 1);
            }
        } else if (checkboxFor(item)->isInside(mouseX, mouseY, x, y, w, h)) {
            togglePending(item);
        }

        return true;
    }

    return true;
}

bool OptionLayer::onMouseMoved(const MouseMovedEvent& event) {
    const auto pos = glm::vec2{ event.getX(), event.getY() };

    ui::updateButtonHover(returnButton.get(), pos);

    hoveredItem = std::nullopt;
    for (const auto& [item, label] : settingRows) {
        const auto [x, y, w, h] = rowLayout(item);
        if (contains(x, y, w, h, pos.x, pos.y)) {
            hoveredItem = item;
            break;
        }
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

        const auto  checkboxSize       = static_cast<float>(fontSize);
        const float maxCycleValueWidth = computeMaxCycleValueWidth();

        for (const auto& [item, label] : settingRows) {
            if (auto* const list = listFor(item); list != nullptr) {
                list->setFontSize(fontSize);
                list->setMaxValueWidth(maxCycleValueWidth);
            } else {
                checkboxFor(item)->setSize(checkboxSize);
            }
        }
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
    filteredResolutions = std::vector(filtered.begin(), filtered.end());

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
        const auto it =
            std::ranges::find_if(filteredResolutions, [&](const auto& r) {
                return r.width == currentWidth && r.height == currentHeight;
            });
        const auto idx = it != filteredResolutions.end() ?
                             std::optional{ static_cast<size_t>(std::distance(
                                 filteredResolutions.begin(), it)) } :
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
    Maze::get().requestVerticalSync(pendingVsync);
    Maze::get().setFxaaEnabled(pendingFxaa);
    const auto shadowRes =
        shadowResolutions[static_cast<size_t>(pendingShadowResIndex)];
    Maze::get().getMazeLayer()->setShadowMapRes(shadowRes);
    saveVideoSettings(saveWidth, saveHeight, pendingFullscreen, pendingVsync,
                      pendingFxaa, shadowRes);

    syncPendingCheckboxState();
    hasUnappliedChanges = false;
    returnButton->setMessage(returnMessage);
}

void OptionLayer::syncPendingCheckboxState() {
    pendingFullscreen = Maze::get().isFullscreen();
    pendingVsync      = Maze::get().hasVerticalSync();
    pendingFxaa       = Maze::get().isFxaaEnabled();

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
    appliedShadowResIndex = pendingShadowResIndex;
}

void OptionLayer::updateChangeStatus() {
    const bool checkboxChanged =
        pendingFullscreen != Maze::get().isFullscreen() ||
        pendingVsync != Maze::get().hasVerticalSync() ||
        pendingFxaa != Maze::get().isFxaaEnabled();

    const auto window = Maze::get().getWindow();
    const auto curW   = window->getWidth();
    const auto curH   = window->getHeight();

    if (!validAspectRatioFilters.empty()) {
        const auto idx = findAspectRatioIndex(curW, curH);
        currentAspectRatioIndex =
            matchesAspectRatio(validAspectRatioFilters[idx], curW, curH) ?
                std::optional{ idx } :
                std::nullopt;
    } else {
        currentAspectRatioIndex = std::nullopt;
    }

    bool resolutionChanged = false;
    if (!filteredResolutions.empty()) {
        const auto& res =
            filteredResolutions[resolutionList->getSelectedIndex()];
        resolutionChanged = res.width != curW || res.height != curH;

        const auto it =
            std::ranges::find_if(filteredResolutions, [&](const auto& r) {
                return r.width == curW && r.height == curH;
            });
        currentResolutionIndex =
            it != filteredResolutions.end() ?
                std::optional{ static_cast<size_t>(
                    std::distance(filteredResolutions.begin(), it)) } :
                std::nullopt;
    }

    const auto currentShadowRes =
        Maze::get().getMazeLayer()->getDirectionalLightShadowMapRes();
    const auto sqIt = std::ranges::find(shadowResolutions, currentShadowRes);
    currentShadowResIndex    = sqIt != shadowResolutions.end() ?
                                   std::optional{ static_cast<size_t>(
                                       sqIt - shadowResolutions.begin()) } :
                                   std::nullopt;
    const bool shadowChanged = pendingShadowResIndex != appliedShadowResIndex;

    hasUnappliedChanges = checkboxChanged || resolutionChanged || shadowChanged;
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
