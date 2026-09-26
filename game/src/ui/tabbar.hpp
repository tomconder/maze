#pragma once

#include "platform/opengl/scene/bitmapfont.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace game::ui {

// The options screen is split into one layer per tab; the tab bar is the only
// thing they share.
enum class OptionTab : uint8_t { Display = 0, Keyboard, Audio, Count };

constexpr std::array<std::string_view, static_cast<size_t>(OptionTab::Count)>
    optionTabLabels{ "Display", "Keyboard", "Audio" };

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

// Draws the tabs across the top of the window, underlining the active one and
// bracketing them with the prompts that switch tab. The strip starts at left,
// which callers pass from their first row so the two line up. Shares the
// caller's font and quad shaders, so the caller must already have set their
// projection this frame.
void renderTabBar(
    OptionTab                                                           active,
    const std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>& font,
    const glm::mat4& projection, float windowWidth, float left);

// Tab under a point, if any. left must match the value given to renderTabBar.
std::optional<OptionTab> tabBarHitTest(
    const std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>& font,
    float windowWidth, float left, const glm::vec2& position);

// Hands the screen to the layer that owns the tab.
void showOptionTab(OptionTab tab);

}  // namespace game::ui
