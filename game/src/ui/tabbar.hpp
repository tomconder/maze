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
enum class OptionTab : uint8_t { Display = 0, Keyboard, Count };

constexpr std::array<std::string_view, static_cast<size_t>(OptionTab::Count)>
    optionTabLabels{ "Display", "Keyboard" };

// Vertical space the bar occupies; reserve it at the top of layouts.
float tabBarHeight(float windowWidth);

// Draws the tabs across the top of the window, underlining the active one and
// bracketing them with the prompts that switch tab. Positioned from the window
// size, not the Yoga tree, so it survives resize without relayout. Shares the
// caller's font and quad shaders, so the caller must already have set their
// projection this frame.
void renderTabBar(
    OptionTab                                                           active,
    const std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>& font,
    const glm::mat4& projection, float windowWidth);

// Tab under a point, if any.
std::optional<OptionTab> tabBarHitTest(
    const std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>& font,
    float windowWidth, const glm::vec2& position);

// Hands the screen to the layer that owns the tab.
void showOptionTab(OptionTab tab);

}  // namespace game::ui
