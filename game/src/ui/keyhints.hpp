#pragma once

#include "platform/opengl/scene/bitmapfont.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <span>
#include <string_view>

namespace game::ui {

// Icons are Kenney input prompt sprites in assets/textures/prompts, named by
// file stem.
struct KeyHint {
    std::string_view keyIcon;  // keyboard prompt, e.g. "keyboard_escape"
    std::string_view padIcon;  // gamepad prompt for the same action
    std::string_view label;    // what it does, e.g. "Back"
};

// Vertical space the bar occupies; reserve it in layouts that reach the
// bottom of the window.
float keyHintBarHeight(float windowWidth);

// windowHeight less the bar, floored at zero: the window can be shorter than
// the bar, and Yoga must never be handed a negative dimension.
float heightWithoutKeyHints(float windowWidth, float windowHeight);

// Draws a row of input prompts along the bottom-left of the window. Positioned
// from the window size, not the Yoga tree, so it survives resize without
// relayout. Shows the gamepad prompt while the gamepad is the device in use.
// rightText, if given, is drawn on the same line against the right margin.
void renderKeyHints(
    std::span<const KeyHint>                                            hints,
    const std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>& font,
    const glm::mat4& projection, float windowWidth, float windowHeight,
    std::string_view rightText = {});

}  // namespace game::ui
