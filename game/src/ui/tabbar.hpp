#pragma once

#include "platform/opengl/scene/bitmapfont.hpp"
#include "ui/layouttypes.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace game::ui {

// The options screen is split into one layer per tab; the tab bar is the only
// thing they share.
enum class OptionTab : uint8_t { Display = 0, Keyboard, Audio, Count };

constexpr size_t tabCount = static_cast<size_t>(OptionTab::Count);

constexpr std::array<std::string_view, tabCount> optionTabLabels{
    "Display",
    "Keyboard",
    "Audio",
};

// Where the tab bar nodes of a layout ended up, in window pixels: the prompt
// that goes to the previous tab, the three labels and the next tab prompt.
struct TabBarRects {
    Rect                       prev;
    std::array<Rect, tabCount> labels;
    Rect                       next;
};

// Next (direction = 1) or previous (direction = -1) tab, wrapping at both
// ends. Any direction works: the second % folds a negative remainder back.
constexpr OptionTab cycleTab(const OptionTab current, const int direction) {
    constexpr auto count = static_cast<int>(OptionTab::Count);
    const auto     next =
        (((static_cast<int>(current) + direction) % count) + count) % count;
    return static_cast<OptionTab>(next);
}

// Vertical space the bar occupies; reserve it at the top of layouts.
float tabBarHeight(float windowWidth);

// Size of the prompt icons, and the gap between the nodes of the bar.
float tabIconSize(float windowWidth);

float tabGap(float windowWidth);

// The size of every tab label at the menu font size for the window width.
std::array<TextSize, tabCount> tabLabelSizes(float              windowWidth,
                                             const TextMeasure& measure);

// Draws the tabs where the layout put them, marking the active one and
// bracketing them with the prompts that switch tab. Shares the caller's font
// and quad shaders, so the caller must already have set their projection this
// frame.
void renderTabBar(
    OptionTab                                                           active,
    const std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>& font,
    const glm::mat4& projection, float windowWidth, const TabBarRects& rects);

// Tab under a point, if any.
std::optional<OptionTab> tabBarHitTest(const TabBarRects& rects,
                                       float              windowWidth,
                                       const glm::vec2&   position);

// Hands the screen to the layer that owns the tab.
void showOptionTab(OptionTab tab);

}  // namespace game::ui
