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
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "core/settings.hpp"
#include "core/window.hpp"
#include "input/gameaction.hpp"
#include "maze.hpp"
#include "platform/glfw/core/application.hpp"
#include "thread/mazeframe.hpp"
#include "ui/button.hpp"
#include "ui/checkbox.hpp"
#include "ui/keyhints.hpp"
#include "ui/rowparts.hpp"
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

constexpr float confirmSeconds = 15.F;

constexpr size_t shadowQualityCount =
    static_cast<size_t>(game::layer::ShadowQuality::Count);

constexpr glm::vec3 textColor          = { 1.F, 1.F, 1.F };
constexpr glm::vec3 arrowDisabledColor = { 0.4F, 0.4F, 0.4F };
constexpr glm::vec3 headingColor       = { 0.6F, 0.6F, 0.6F };

constexpr glm::vec4 dotColorDefault = { 0.25F, 0.25F, 0.25F, 1.F };
constexpr glm::vec4 dotColorCurrent = { 1.F, 1.F, 1.F, 1.F };
constexpr glm::vec4 dotColorDisplay = { 0.65F, 0.65F, 0.65F, 1.F };

constexpr float textMarginLeft = 26.F;

// The slider holds 0 to 1; the frame rate is that across the range.
constexpr float frameRateSpan =
    static_cast<float>(game::layer::maxFrameRate - game::layer::minFrameRate);

// One key press or arrow click moves the frame rate slider 5 frames a second.
constexpr float frameRateSliderStep = 5.F / frameRateSpan;

float frameRateToSlider(const uint32_t fps) {
    const auto clamped =
        std::clamp(fps, game::layer::minFrameRate, game::layer::maxFrameRate);
    return static_cast<float>(clamped - game::layer::minFrameRate) /
           frameRateSpan;
}

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

// Index order is the sponge::core::DisplayMode order.
constexpr std::array<std::string_view, 3> displayModeLabels = {
    "Windowed", "Full Screen", "Borderless Window"
};
static_assert(displayModeLabels.size() ==
              static_cast<size_t>(sponge::core::DisplayMode::Count));

// max display width across all aspect-ratio labels and resolution strings.
// Measured with tabularFigures=true to match how they render.
float computeMaxCycleValueWidth(
    const std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>&
                   menuFont,
    const uint32_t fontSize) {
    float width = std::accumulate(
        validAspectRatioFilters.begin(), validAspectRatioFilters.end(), 0.F,
        [&](const float acc, const auto& f) {
            return std::max(acc, static_cast<float>(menuFont->getLength(
                                     f.label, fontSize, true)));
        });
    for (const auto label : displayModeLabels) {
        width = std::max(
            width, static_cast<float>(menuFont->getLength(label, fontSize)));
    }
    return std::accumulate(
        availableResolutions.begin(), availableResolutions.end(), width,
        [&](const float acc, const auto& res) {
            return std::max(acc,
                            static_cast<float>(menuFont->getLength(
                                fmt::format("{} × {}", res.width, res.height),
                                fontSize, true)));
        });
}

// The display mode a change replaced, until the player accepts it.
std::optional<sponge::core::DisplayMode> previousDisplayMode;

using game::layer::OptionMenuItem;

constexpr size_t rowCount = static_cast<size_t>(OptionMenuItem::Count);

struct RowDef {
    OptionMenuItem   item;
    std::string_view label;
};

// A heading is a label only: it is never selected, hovered or clicked.
constexpr bool isHeading(const OptionMenuItem item) {
    return item == OptionMenuItem::GeneralHeading ||
           item == OptionMenuItem::QualityHeading ||
           item == OptionMenuItem::AdvancedHeading;
}

constexpr std::array settingRows = {
    RowDef{ .item = OptionMenuItem::GeneralHeading, .label = "General" },
    RowDef{ .item = OptionMenuItem::DisplayMode, .label = "Display Mode" },
    RowDef{ .item = OptionMenuItem::AspectRatio, .label = "Aspect Ratio" },
    RowDef{ .item = OptionMenuItem::Resolution, .label = "Resolution" },
    RowDef{ .item = OptionMenuItem::VerticalSync, .label = "VSync" },
    RowDef{ .item = OptionMenuItem::FrameRateCap, .label = "Frame Rate Cap" },
    RowDef{ .item  = OptionMenuItem::MaxFrameRate,
            .label = "Maximum Frame Rate" },
    RowDef{ .item = OptionMenuItem::AntiAliasing, .label = "Anti-aliasing" },
    RowDef{ .item = OptionMenuItem::QualityHeading, .label = "Quality" },
    RowDef{ .item = OptionMenuItem::ShadowQuality, .label = "Shadow Quality" },
    RowDef{ .item = OptionMenuItem::AdvancedHeading, .label = "Advanced" },
    RowDef{ .item  = OptionMenuItem::AmbientOcclusion,
            .label = "Ambient Occlusion" },
    RowDef{ .item = OptionMenuItem::Bloom, .label = "Bloom" },
    RowDef{ .item  = OptionMenuItem::SubsurfaceScattering,
            .label = "Subsurface Scattering" },
};

// rowNodes are created by index, so the table must stay in enum order and
// cover every row.
static_assert(settingRows.size() == rowCount);
static_assert([] {
    for (size_t i = 0; i < settingRows.size(); i++) {
        if (static_cast<size_t>(settingRows[i].item) != i) {
            return false;
        }
    }
    return true;
}());

std::optional<size_t> currentDisplayModeIndex;
std::optional<size_t> currentShadowQualityIndex;
std::optional<size_t> currentAaIndex;

// Index order is the OptionMenuItem row order the SelectList shows.
constexpr std::array aaModes = {
    game::thread::AntiAliasing::None,
    game::thread::AntiAliasing::Fxaa,
    game::thread::AntiAliasing::Taa,
};
static_assert(aaModes.size() ==
              static_cast<size_t>(game::thread::AntiAliasing::Count));

// Dot strip state for the cycling rows: total items, applied index.
std::pair<size_t, std::optional<size_t>> dotState(const OptionMenuItem item) {
    switch (item) {
        case OptionMenuItem::AspectRatio:
            return { validAspectRatioFilters.size(), currentAspectRatioIndex };
        case OptionMenuItem::Resolution:
            return { filteredResolutions.size(), currentResolutionIndex };
        case OptionMenuItem::DisplayMode:
            return { displayModeLabels.size(), currentDisplayModeIndex };
        case OptionMenuItem::AntiAliasing:
            return { aaModes.size(), currentAaIndex };
        case OptionMenuItem::ShadowQuality:
            return { shadowQualityCount, currentShadowQualityIndex };
        default:
            return { 0, std::nullopt };
    }
}

game::layer::ScrollMenuStyle optionStyle() {
    return {
        .name     = "options",
        .tab      = game::ui::OptionTab::Display,
        .rowCount = rowCount,
        .keyHints = game::layer::adjustKeyHints,
        .firstRow = static_cast<size_t>(OptionMenuItem::DisplayMode),
    };
}
}  // namespace

namespace game::layer {

OptionLayer::OptionLayer() : ScrollMenuLayer(optionStyle()) {}

void OptionLayer::onAttach() {
    ScrollMenuLayer::onAttach();

    availableResolutions =
        sponge::platform::glfw::core::Application::getAvailableResolutions();

    validAspectRatioFilters.clear();
    for (const auto& f : aspectRatioFilters) {
        if (hasMatchingResolution(f, availableResolutions)) {
            validAspectRatioFilters.push_back(f);
        }
    }

    const float maxCycleValueWidth =
        computeMaxCycleValueWidth(menuFont(), menuFontSize());

    const ui::SelectListCreateInfo selectCreateInfo{
        .font               = menuFont(),
        .fontSize           = menuFontSize(),
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
    displayModeList   = std::make_unique<ui::SelectList>(selectCreateInfo);
    shadowQualityList = std::make_unique<ui::SelectList>(selectCreateInfo);
    antiAliasingList  = std::make_unique<ui::SelectList>(selectCreateInfo);

    const ui::CheckboxCreateInfo checkboxCreateInfo{
        .margin = textMarginLeft,
        .size   = static_cast<float>(menuFontSize()),
    };
    verticalSyncCheckbox = std::make_unique<ui::Checkbox>(checkboxCreateInfo);
    frameRateCapCheckbox = std::make_unique<ui::Checkbox>(checkboxCreateInfo);
    ambientOcclusionCheckbox =
        std::make_unique<ui::Checkbox>(checkboxCreateInfo);
    bloomCheckbox = std::make_unique<ui::Checkbox>(checkboxCreateInfo);
    subsurfaceScatteringCheckbox =
        std::make_unique<ui::Checkbox>(checkboxCreateInfo);

    std::vector<std::string> arItems;
    arItems.reserve(validAspectRatioFilters.size());
    for (const auto& f : validAspectRatioFilters) {
        arItems.emplace_back(f.label);
    }
    aspectRatioList->setItems(std::move(arItems));

    displayModeList->setItems(
        { displayModeLabels.begin(), displayModeLabels.end() });
    shadowQualityList->setItems({ "Low", "Medium", "High" });
    antiAliasingList->setItems({ "Off", "FXAA", "TAA" });

    const auto window        = Maze::get().getWindow();
    const auto currentWidth  = window->getWidth();
    const auto currentHeight = window->getHeight();

    for (size_t row = 0; row < settingRows.size(); row++) {
        if (auto* const list = listFor(settingRows[row].item);
            list != nullptr) {
            list->attach(rowNode(row));
        }
    }

    // One step of indent: the slider belongs to the checkbox above it.
    frameRateSlider = std::make_unique<ui::Slider>(ui::SliderCreateInfo{
        .font               = menuFont(),
        .fontSize           = menuFontSize(),
        .textColor          = textColor,
        .arrowDisabledColor = arrowDisabledColor,
        .textMarginLeft     = textMarginLeft,
        .labelIndent        = 32.F,
    });
    frameRateSlider->attach(
        rowNode(static_cast<size_t>(OptionMenuItem::MaxFrameRate)));

    // A heading is a label alone; a checkbox row is a label and the box.
    for (size_t row = 0; row < settingRows.size(); row++) {
        const auto item = settingRows[row].item;
        if (isHeading(item)) {
            ui::attachRowParts(rowNode(row), 1, textMarginLeft);
        } else if (auto* const box = checkboxFor(item); box != nullptr) {
            box->attach(rowNode(row));
        }
    }
    onFontSizeChanged();

    syncPendingCheckboxState();

    aspectRatioList->setSelectedIndex(
        findAspectRatioIndex(currentWidth, currentHeight));

    filterResolutions();
    recalculateLayout();
}

bool OptionLayer::takesInput() const {
    // the prompt over this screen takes the keys while it is open
    return !Maze::get().getConfirmLayer()->isActive();
}

bool OptionLayer::isSelectable(const size_t row) const {
    const auto item = static_cast<OptionMenuItem>(row);
    // The slider is a label until the cap is on.
    return !isHeading(item) &&
           (item != OptionMenuItem::MaxFrameRate || pendingFrameRateCap);
}

void OptionLayer::onOpened() {
    resetSelectionToCurrentState();
}

void OptionLayer::onClosed() {
    resetSelectionToCurrentState();
}

void OptionLayer::renderDots(const size_t count, const float valueCenterX,
                             const float rowY, const float rowH,
                             const size_t                 displayIdx,
                             const std::optional<size_t>& currentIdx) const {
    if (count == 0) {
        return;
    }
    const float dotRadius =
        std::max(3.F, std::round(static_cast<float>(menuFontSize()) / 12.F));
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
        menuQuad().render({ dotX, dotY },
                          { dotX + dotRadius * 2.F, dotY + dotRadius * 2.F },
                          color, dotRadius);
        dotX += dotSpacing;
    }
}

void OptionLayer::renderRowLabel(const size_t row, const float y,
                                 const std::string_view label,
                                 const glm::vec3&       color) const {
    const auto& labelFont = menuFont();
    const auto  rect      = ui::partRect(rowNode(row), ui::labelPart, y);
    labelFont->beginPass(menuFontSize());
    labelFont->render(label, { rect.x, rect.y }, color);
    labelFont->endPass();
}

void OptionLayer::renderRow(const size_t row, const float /*x*/, const float y,
                            const float /*w*/, const float h) {
    const auto& [item, label] = settingRows[row];
    if (isHeading(item)) {
        renderRowLabel(row, y, label, headingColor);
    } else if (item == OptionMenuItem::MaxFrameRate) {
        frameRateSlider->onUpdate(y, label);
    } else if (auto* const list = listFor(item); list != nullptr) {
        list->onUpdate(y, label);
        const auto [dotCount, appliedIndex] = dotState(item);
        renderDots(dotCount, list->getValueCenterX(), y, h,
                   list->getSelectedIndex(), appliedIndex);
    } else {
        renderRowLabel(row, y, label, textColor);
        checkboxFor(item)->onUpdate(y, *pendingFor(item));
    }
}

void OptionLayer::onRowKeys(const size_t                        row,
                            const sponge::input::InputSnapshot& input) {
    using sponge::input::GameAction;
    const auto item = static_cast<OptionMenuItem>(row);
    if (item == OptionMenuItem::MaxFrameRate) {
        bool changed = false;
        if (input.isActive(GameAction::MenuLeft)) {
            changed = frameRateSlider->step(-frameRateSliderStep);
        }
        if (input.isActive(GameAction::MenuRight)) {
            changed = frameRateSlider->step(frameRateSliderStep) || changed;
        }
        if (changed) {
            applyFrameRate(true);
        }
        return;
    }
    if (input.isActive(GameAction::MenuLeft)) {
        cycleList(item, -1);
    }
    if (input.isActive(GameAction::MenuRight)) {
        cycleList(item, 1);
    }
}

bool OptionLayer::onConfirm(const size_t row) {
    togglePending(static_cast<OptionMenuItem>(row));
    return true;
}

bool OptionLayer::onRowPressed(const size_t row, const glm::vec2& mouse) {
    const auto y    = std::get<1>(rowRect(row));
    const auto item = static_cast<OptionMenuItem>(row);
    if (item == OptionMenuItem::MaxFrameRate) {
        auto& slider = *frameRateSlider;
        if (slider.isInsideTrack(mouse.x)) {
            slider.setValue(slider.valueAtX(mouse.x));
            applyFrameRate(false);
            return true;
        }
        if (const auto arrow = slider.arrowAt(mouse.x); arrow != 0) {
            if (slider.step(static_cast<float>(arrow) * frameRateSliderStep)) {
                applyFrameRate(true);
            }
        }
        return false;
    }
    if (auto* const list = listFor(item); list != nullptr) {
        if (list->isInsideLeft(mouse.x)) {
            cycleList(item, -1);
        } else if (list->isInsideRight(mouse.x)) {
            cycleList(item, 1);
        }
    } else if (checkboxFor(item)->isInside(mouse.x, mouse.y, y)) {
        togglePending(item);
    }
    return false;
}

void OptionLayer::onRowDrag(const size_t /*row*/, const glm::vec2& mouse) {
    frameRateSlider->setValue(frameRateSlider->valueAtX(mouse.x));
    applyFrameRate(false);
}

void OptionLayer::onRowReleased(const size_t /*row*/) {
    applyFrameRate(true);
}

void OptionLayer::onFontSizeChanged() {
    const auto checkboxSize = static_cast<float>(menuFontSize());
    const auto textHeight =
        static_cast<float>(menuFont()->getHeight(menuFontSize()));
    const float maxCycleValueWidth =
        computeMaxCycleValueWidth(menuFont(), menuFontSize());

    for (size_t row = 0; row < settingRows.size(); row++) {
        const auto item = settingRows[row].item;
        if (auto* const list = listFor(item); list != nullptr) {
            list->setFontSize(menuFontSize());
            list->setMaxValueWidth(maxCycleValueWidth);
        } else if (item == OptionMenuItem::MaxFrameRate) {
            frameRateSlider->setFontSize(menuFontSize());
        } else {
            if (!isHeading(item)) {
                checkboxFor(item)->setSize(checkboxSize);
            }
            ui::setPartSize(rowNode(row), ui::labelPart, YGUndefined,
                            textHeight);
        }
    }
}

void OptionLayer::onResized(const uint32_t width, const uint32_t height) {
    // A mode change lands here, so the rows follow the window. The OS can
    // clamp a window to the desktop work area; that size is in no list, and
    // following it would drop the row to the first 4:3 entry.
    if (isActive()) {
        const bool listed =
            std::ranges::any_of(availableResolutions, [&](const auto& res) {
                return res.width == width && res.height == height;
            });
        if (listed) {
            resetSelectionToCurrentState();
        } else {
            updateChangeStatus();
        }
    }
}

ui::SelectList* OptionLayer::listFor(const OptionMenuItem item) const {
    switch (item) {
        case OptionMenuItem::AspectRatio:
            return aspectRatioList.get();
        case OptionMenuItem::Resolution:
            return resolutionList.get();
        case OptionMenuItem::DisplayMode:
            return displayModeList.get();
        case OptionMenuItem::AntiAliasing:
            return antiAliasingList.get();
        case OptionMenuItem::ShadowQuality:
            return shadowQualityList.get();
        default:
            return nullptr;
    }
}

ui::Checkbox* OptionLayer::checkboxFor(const OptionMenuItem item) const {
    switch (item) {
        case OptionMenuItem::VerticalSync:
            return verticalSyncCheckbox.get();
        case OptionMenuItem::FrameRateCap:
            return frameRateCapCheckbox.get();
        case OptionMenuItem::AmbientOcclusion:
            return ambientOcclusionCheckbox.get();
        case OptionMenuItem::Bloom:
            return bloomCheckbox.get();
        case OptionMenuItem::SubsurfaceScattering:
            return subsurfaceScatteringCheckbox.get();
        default:
            return nullptr;
    }
}

bool* OptionLayer::pendingFor(const OptionMenuItem item) {
    switch (item) {
        case OptionMenuItem::VerticalSync:
            return &pendingVsync;
        case OptionMenuItem::FrameRateCap:
            return &pendingFrameRateCap;
        case OptionMenuItem::AmbientOcclusion:
            return &pendingAmbientOcclusion;
        case OptionMenuItem::Bloom:
            return &pendingBloom;
        case OptionMenuItem::SubsurfaceScattering:
            return &pendingSubsurfaceScattering;
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

    if (!(delta < 0 ? list->selectPrev() : list->selectNext())) {
        return;
    }
    ui::playHoverClick();

    const auto index = static_cast<int>(list->getSelectedIndex());
    switch (item) {
        case OptionMenuItem::AspectRatio:
            // the new ratio takes its first resolution, applied like a
            // resolution change
            filterResolutions();
            if (!filteredResolutions.empty()) {
                applyResolution();
            }
            break;
        case OptionMenuItem::DisplayMode:
            pendingDisplayModeIndex = index;
            applyDisplayMode();
            break;
        case OptionMenuItem::Resolution:
            applyResolution();
            break;
        case OptionMenuItem::ShadowQuality: {
            pendingShadowQualityIndex = index;
            const auto quality        = static_cast<ShadowQuality>(index);
            Maze::get().getMazeLayer()->setShadowQuality(quality);
            sponge::core::Settings::set("video.shadowQuality",
                                        static_cast<uint32_t>(quality));
            sponge::core::Settings::save();
            break;
        }
        case OptionMenuItem::AntiAliasing: {
            pendingAaIndex    = index;
            const auto aaMode = aaModes[static_cast<size_t>(index)];
            Maze::get().setAntiAliasing(aaMode);
            sponge::core::Settings::set("video.aa",
                                        static_cast<uint32_t>(aaMode));
            sponge::core::Settings::save();
            break;
        }
        default:
            break;
    }
    updateChangeStatus();
}

void OptionLayer::togglePending(const OptionMenuItem item) {
    auto* const pending = pendingFor(item);
    if (pending == nullptr) {
        return;
    }
    *pending = !*pending;

    // Vsync and the effects below apply on the spot and need no confirmation.
    switch (item) {
        case OptionMenuItem::VerticalSync:
            Maze::get().requestVerticalSync(pendingVsync);
            sponge::core::Settings::set("video.vsync", pendingVsync);
            break;
        case OptionMenuItem::FrameRateCap:
            Maze::get().setFrameRateCap(pendingFrameRateCap ? frameRate() : 0);
            sponge::core::Settings::set("video.frameRateCap",
                                        pendingFrameRateCap);
            break;
        case OptionMenuItem::AmbientOcclusion:
            Maze::get().getMazeLayer()->setSsaoEnabled(pendingAmbientOcclusion);
            sponge::core::Settings::set("video.ssaoEnabled",
                                        pendingAmbientOcclusion);
            break;
        case OptionMenuItem::Bloom:
            Maze::get().setBloomEnabled(pendingBloom);
            sponge::core::Settings::set("video.bloomEnabled", pendingBloom);
            break;
        case OptionMenuItem::SubsurfaceScattering:
            Maze::get().getMazeLayer()->setTranslucencyEnabled(
                pendingSubsurfaceScattering);
            sponge::core::Settings::set("video.translucencyEnabled",
                                        pendingSubsurfaceScattering);
            break;
        default:
            break;
    }
    sponge::core::Settings::save();

    updateChangeStatus();
}

uint32_t OptionLayer::frameRate() const {
    return minFrameRate + static_cast<uint32_t>(std::lround(
                              frameRateSlider->getValue() * frameRateSpan));
}

void OptionLayer::applyFrameRate(const bool save) {
    const auto fps = frameRate();
    frameRateSlider->setValueText(std::to_string(fps));
    if (pendingFrameRateCap) {
        Maze::get().setFrameRateCap(fps);
    }
    sponge::core::Settings::set("video.maxFrameRate", fps);
    if (save) {
        sponge::core::Settings::save();
    }
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

void OptionLayer::applyDisplayMode() {
    previousDisplayMode = Maze::get().getDisplayMode();
    Maze::get().setDisplayMode(
        static_cast<sponge::core::DisplayMode>(pendingDisplayModeIndex));
    clearHoveredItems();
    Maze::get().getConfirmLayer()->show(
        displayChangeTitle, displayChangeMessage,
        [this](const ui::ConfirmResult result) {
            resolveDisplayChange(result);
        },
        confirmSeconds);
}

// A size cannot lock the player out, so it applies and saves with no prompt.
void OptionLayer::applyResolution() {
    const auto& res = filteredResolutions[resolutionList->getSelectedIndex()];
    Maze::get().setResolution(res.width, res.height);
    sponge::core::Settings::set("video.width", res.width);
    sponge::core::Settings::set("video.height", res.height);
    sponge::core::Settings::save();
}

void OptionLayer::resolveDisplayChange(const ui::ConfirmResult result) {
    if (result == ui::ConfirmResult::Pending) {
        return;
    }
    if (result == ui::ConfirmResult::Accepted) {
        sponge::core::Settings::set(
            "video.displayMode",
            static_cast<uint32_t>(Maze::get().getDisplayMode()));
        sponge::core::Settings::save();
    } else if (previousDisplayMode) {
        Maze::get().setDisplayMode(*previousDisplayMode);
    }
    previousDisplayMode.reset();

    resetSelectionToCurrentState();
}

void OptionLayer::syncPendingCheckboxState() {
    pendingDisplayModeIndex = static_cast<int>(Maze::get().getDisplayMode());
    if (displayModeList) {
        displayModeList->setSelectedIndex(
            static_cast<size_t>(pendingDisplayModeIndex));
    }
    pendingVsync = Maze::get().hasVerticalSync();
    // Settings holds the cap and its value; the slider keeps its own value
    // while the cap is off.
    pendingFrameRateCap =
        sponge::core::Settings::getBool("video.frameRateCap", false);
    frameRateSlider->setValue(
        frameRateToSlider(sponge::core::Settings::getUInt32(
            "video.maxFrameRate", defaultFrameRate)));
    frameRateSlider->setValueText(std::to_string(frameRate()));
    pendingAmbientOcclusion = Maze::get().getMazeLayer()->isSsaoEnabled();
    pendingBloom            = Maze::get().isBloomEnabled();
    pendingSubsurfaceScattering =
        Maze::get().getMazeLayer()->isTranslucencyEnabled();

    const auto aaIt = std::ranges::find(aaModes, Maze::get().getAntiAliasing());
    pendingAaIndex =
        aaIt != aaModes.end() ? static_cast<int>(aaIt - aaModes.begin()) : 0;
    if (antiAliasingList) {
        antiAliasingList->setSelectedIndex(static_cast<size_t>(pendingAaIndex));
    }

    pendingShadowQualityIndex =
        static_cast<int>(Maze::get().getMazeLayer()->getShadowQuality());
    if (shadowQualityList) {
        shadowQualityList->setSelectedIndex(
            static_cast<size_t>(pendingShadowQualityIndex));
    }
}

void OptionLayer::updateChangeStatus() {
    currentDisplayModeIndex = static_cast<size_t>(Maze::get().getDisplayMode());

    // A borderless window has the desktop size, so the size rows are locked.
    const bool borderless =
        pendingDisplayModeIndex ==
        static_cast<int>(sponge::core::DisplayMode::Borderless);
    aspectRatioList->setDisabled(borderless);
    resolutionList->setDisabled(borderless);
    frameRateSlider->setDisabled(!pendingFrameRateCap);

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

    if (!filteredResolutions.empty()) {
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

    currentShadowQualityIndex =
        static_cast<size_t>(Maze::get().getMazeLayer()->getShadowQuality());

    const auto aaIt = std::ranges::find(aaModes, Maze::get().getAntiAliasing());
    currentAaIndex =
        aaIt != aaModes.end() ?
            std::optional{ static_cast<size_t>(aaIt - aaModes.begin()) } :
            std::nullopt;
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
