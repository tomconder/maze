#include "layer/optionrows.hpp"

#include <algorithm>
#include <cstddef>

#include "ui/menulayout.hpp"
#include "ui/rowlayout.hpp"

namespace game::layer {

namespace {
// The widest value any cycling row shows, so the arrows stay put. The aspect
// ratios and resolutions are measured with tabular figures, as they draw.
float maxCycleValueWidth(const ui::RowText& text, const OptionRowData& data) {
    float width = 0.F;
    for (const auto label : data.aspectLabels) {
        width = std::max(width, text.measure(label, text.fontSize, true).width);
    }
    for (const auto label : displayModeLabels) {
        width = std::max(width, text.measure(label, text.fontSize).width);
    }
    for (const auto& label : data.resolutionLabels) {
        width = std::max(width, text.measure(label, text.fontSize, true).width);
    }
    return width;
}

size_t dotCount(const OptionMenuItem item, const OptionRowData& data) {
    switch (item) {
        case OptionMenuItem::AspectRatio:
            return data.aspectLabels.size();
        case OptionMenuItem::Resolution:
            return data.resolutionDots;
        case OptionMenuItem::DisplayMode:
            return displayModeLabels.size();
        case OptionMenuItem::AntiAliasing:
            return aaModeCount;
        case OptionMenuItem::ShadowQuality:
            return shadowQualityCount;
        default:
            return 0;
    }
}
}  // namespace

void layoutOptionRows(const ui::MenuLayout& layout, const ui::RowText& text,
                      const OptionRowData& data) {
    const auto valueWidth = maxCycleValueWidth(text, data);
    for (size_t row = 0; row < optionRows.size(); row++) {
        const auto  item = optionRows[row].item;
        auto* const node = layout.rowNode(row);
        switch (optionRows[row].kind) {
            case OptionRowKind::Heading:
                ui::layoutHeading(node, text);
                break;
            case OptionRowKind::Cycle:
                ui::layoutSelect(node, text, valueWidth, dotCount(item, data));
                break;
            case OptionRowKind::Checkbox:
                ui::layoutCheckbox(node, text,
                                   static_cast<float>(text.fontSize));
                break;
            case OptionRowKind::Slider:
                // One step of indent: the slider belongs to the checkbox
                // above it.
                ui::layoutSlider(node, text, 32.F);
                break;
        }
    }
}

}  // namespace game::layer
