#pragma once

#include "platform/opengl/scene/bitmapfont.hpp"

#include <memory>
#include <span>
#include <string_view>

namespace game::ui {

struct KeyHint {
    std::string_view key;    // key cap text, e.g. "Esc"
    std::string_view pad;    // gamepad button for the same action, e.g. "B"
    std::string_view label;  // what it does, e.g. "Back"
};

// Vertical space the bar occupies; reserve it in layouts that reach the
// bottom of the window.
float keyHintBarHeight(float windowWidth);

// Draws a row of key caps along the bottom-left of the window. Positioned from
// the window size, not the Yoga tree, so it survives resize without relayout.
// Shows the gamepad button while the gamepad is the device in use.
void renderKeyHints(
    std::span<const KeyHint>                                            hints,
    const std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>& font,
    float windowWidth, float windowHeight);

}  // namespace game::ui
