#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "ui/menulayout.hpp"
#include "ui/rowlayout.hpp"

namespace game::layer {

// The Maximum Frame Rate slider runs over this range, in frames a second.
inline constexpr uint32_t minFrameRate     = 30;
inline constexpr uint32_t maxFrameRate     = 240;
inline constexpr uint32_t defaultFrameRate = 60;

enum class OptionMenuItem : uint8_t {
    GeneralHeading = 0,
    DisplayMode,
    AspectRatio,
    Resolution,
    VerticalSync,
    FrameRateCap,
    MaxFrameRate,
    AntiAliasing,
    QualityHeading,
    ShadowQuality,
    AdvancedHeading,
    AmbientOcclusion,
    Bloom,
    SubsurfaceScattering,
    Count,
};

inline constexpr size_t optionRowCount =
    static_cast<size_t>(OptionMenuItem::Count);

enum class OptionRowKind : uint8_t {
    Heading,  // a label only: never selected, hovered or clicked
    Cycle,    // a select list
    Checkbox,
    Slider,
};

struct OptionRowDef {
    OptionMenuItem   item;
    std::string_view label;
    OptionRowKind    kind;
};

// rowNodes are created by index, so the table stays in enum order and covers
// every row.
inline constexpr std::array optionRows = {
    OptionRowDef{ OptionMenuItem::GeneralHeading, "General",
                  OptionRowKind::Heading },
    OptionRowDef{ OptionMenuItem::DisplayMode, "Display Mode",
                  OptionRowKind::Cycle },
    OptionRowDef{ OptionMenuItem::AspectRatio, "Aspect Ratio",
                  OptionRowKind::Cycle },
    OptionRowDef{ OptionMenuItem::Resolution, "Resolution",
                  OptionRowKind::Cycle },
    OptionRowDef{ OptionMenuItem::VerticalSync, "VSync",
                  OptionRowKind::Checkbox },
    OptionRowDef{ OptionMenuItem::FrameRateCap, "Frame Rate Cap",
                  OptionRowKind::Checkbox },
    OptionRowDef{ OptionMenuItem::MaxFrameRate, "Maximum Frame Rate",
                  OptionRowKind::Slider },
    OptionRowDef{ OptionMenuItem::AntiAliasing, "Anti-aliasing",
                  OptionRowKind::Cycle },
    OptionRowDef{ OptionMenuItem::QualityHeading, "Quality",
                  OptionRowKind::Heading },
    OptionRowDef{ OptionMenuItem::ShadowQuality, "Shadow Quality",
                  OptionRowKind::Cycle },
    OptionRowDef{ OptionMenuItem::AdvancedHeading, "Advanced",
                  OptionRowKind::Heading },
    OptionRowDef{ OptionMenuItem::AmbientOcclusion, "Ambient Occlusion",
                  OptionRowKind::Checkbox },
    OptionRowDef{ OptionMenuItem::Bloom, "Bloom", OptionRowKind::Checkbox },
    OptionRowDef{ OptionMenuItem::SubsurfaceScattering, "Subsurface Scattering",
                  OptionRowKind::Checkbox },
};

static_assert(optionRows.size() == optionRowCount);
static_assert([] {
    for (size_t i = 0; i < optionRows.size(); i++) {
        if (static_cast<size_t>(optionRows[i].item) != i) {
            return false;
        }
    }
    return true;
}());

// Index order is the sponge::core::DisplayMode order.
inline constexpr std::array<std::string_view, 3> displayModeLabels = {
    "Windowed", "Full Screen", "Borderless Window"
};

// One dot per item under the value of the fixed lists. Shadow quality is Low,
// Medium, High; anti-aliasing is Off, FXAA, TAA.
inline constexpr size_t shadowQualityCount = 3;
inline constexpr size_t aaModeCount        = 3;

// What the monitors give the rows. A window supplies the real lists; a layout
// dump without a window supplies stand-ins.
struct OptionRowData {
    // The aspect ratios some resolution matches.
    std::span<const std::string_view> aspectLabels;
    // Every resolution of the monitor as "W × H", for the width of the value.
    std::span<const std::string> resolutionLabels;
    // The resolutions that match the selected aspect ratio.
    size_t resolutionDots = 0;
};

// Builds and sizes every row of the display tab. Needs no window.
void layoutOptionRows(const ui::MenuLayout& layout, const ui::RowText& text,
                      const OptionRowData& data);

}  // namespace game::layer
