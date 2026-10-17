#include "layer/menudump.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include "layer/audiorows.hpp"
#include "layer/confirmlayer.hpp"
#include "layer/exitlayer.hpp"
#include "layer/introlayer.hpp"
#include "layer/keymaprows.hpp"
#include "layer/optionrows.hpp"
#include "layer/scrollmenulayer.hpp"
#include "ui/confirmlayout.hpp"
#include "ui/menufontsize.hpp"
#include "ui/menulayout.hpp"
#include "ui/rowlayout.hpp"
#include "ui/yogadump.hpp"

namespace game::layer {

namespace {
constexpr std::string_view countdownText = "Reverting in 15";

constexpr std::array<std::string_view, 3> aspectStandIns = { "4:3", "16:9",
                                                             "16:10" };

std::string confirmTree(const bool timed, const float width, const float height,
                        const ui::RowText& text) {
    ui::ConfirmLayout layout;
    layout.calculate(width, height, text,
                     timed ? displayChangeTitle : resetBindingsTitle,
                     timed ? displayChangeMessage : resetBindingsMessage,
                     timed ? std::optional{ countdownText } : std::nullopt);
    return ui::yogaToPlayground(layout.root());
}

// A menu is its rows, the key hints and, on an option tab, the tab bar.
template <typename LayoutRows>
std::string menuTree(const size_t rows, const ui::MenuLayoutSpec& spec,
                     const std::span<const ui::KeyHint> hints, const bool tabs,
                     const float width, const float height,
                     const ui::RowText& text, LayoutRows&& layoutRows) {
    ui::MenuLayout layout(rows, spec, hints);
    layoutRows(layout, text);
    if (tabs) {
        layout.setTabLabelSizes(ui::tabLabelSizes(width, text.measure));
    }
    layout.setKeyHintSizes(ui::keyHintSizes(hints, width, false, text.measure),
                           false);
    layout.calculate(width, height);
    return ui::yogaToPlayground(layout.root());
}
}  // namespace

std::optional<std::string> dumpMenuYoga(const std::string_view screen,
                                        const uint32_t         width,
                                        const uint32_t         height,
                                        const ui::TextMeasure& measure) {
    const auto w    = static_cast<float>(width);
    const auto h    = static_cast<float>(height);
    const auto text = ui::RowText{ measure, ui::menuFontSizeForWidth(width) };

    if (screen == "confirm" || screen == "reset") {
        return confirmTree(screen == "confirm", w, h, text);
    }
    if (screen == "intro") {
        return menuTree(introMenuLabels.size(), introMenuLayout, introKeyHints,
                        false, w, h, text,
                        [](const auto& layout, const auto& t) {
                            ui::layoutMenuButtons(layout, t, introMenuLabels);
                        });
    }
    if (screen == "exit") {
        return menuTree(exitMenuLabels.size(), exitMenuLayout, exitKeyHints,
                        false, w, h, text,
                        [](const auto& layout, const auto& t) {
                            ui::layoutMenuButtons(layout, t, exitMenuLabels);
                        });
    }
    if (screen == "keyboard") {
        return menuTree(keyMapRowCount, scrollMenuLayout, keyMapKeyHints, true,
                        w, h, text, layoutKeyMapRows);
    }
    if (screen == "audio") {
        return menuTree(audioRowCount, scrollMenuLayout, adjustKeyHints, true,
                        w, h, text, layoutAudioRows);
    }
    if (screen == "display") {
        const std::array<std::string, 4> resolutions = {
            "1280 × 720", "1920 × 1080", "2560 × 1440", "3840 × 2160"
        };
        return menuTree(optionRowCount, scrollMenuLayout, adjustKeyHints, true,
                        w, h, text, [&](const auto& layout, const auto& t) {
                            layoutOptionRows(
                                layout, t,
                                { .aspectLabels     = aspectStandIns,
                                  .resolutionLabels = resolutions,
                                  .resolutionDots   = resolutions.size() });
                        });
    }
    return std::nullopt;
}

}  // namespace game::layer
