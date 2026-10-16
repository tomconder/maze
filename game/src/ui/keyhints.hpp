#pragma once

#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/sprite.hpp"
#include "ui/layouttypes.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace game::ui {

// Icons are Kenney input prompts, baked into the UI atlas and named by the
// file stem they had in assets/textures/prompts.
struct KeyHint {
    std::string_view keyIcon;  // keyboard prompt, e.g. "keyboard_escape"
    std::string_view padIcon;  // gamepad prompt for the same action
    std::string_view label;    // what it does, e.g. "Back"
    bool leftAligned = false;  // sits beside Back, not at the right margin
};

// Prompt sprite by name, from the UI atlas.
const sponge::platform::opengl::scene::Sprite&
    promptSprite(std::string_view name);

// Vertical space the bar occupies; the layout reserves it along the bottom of
// the window.
float keyHintBarHeight(float windowWidth);

// Space left and right of the bar, and between the hints.
float keyHintMarginX(float windowWidth);

float keyHintGap(float windowWidth);

// True while the gamepad is the device in use; its prompts replace the
// keyboard ones, and some are drawn at another size.
bool usingGamepad();

// Measures a text with the font.
TextMeasure fontMeasure(
    const std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>& font);

// Indices of the hints at the left margin, Back first so it sits in the same
// place on every menu, then the hints marked leftAligned, and of the rest,
// which are right-aligned against the right margin in their order.
struct KeyHintOrder {
    std::vector<size_t> left;
    std::vector<size_t> right;
};

KeyHintOrder keyHintOrder(std::span<const KeyHint> hints);

// The size of the icon as drawn, and of the label, of every hint.
struct KeyHintSize {
    float    icon;
    TextSize label;
};

std::vector<KeyHintSize> keyHintSizes(std::span<const KeyHint> hints,
                                      float windowWidth, bool gamepad,
                                      const TextMeasure& measure);

// Where the nodes of a hint ended up, in window pixels.
struct KeyHintRects {
    Rect hint;
    Rect icon;
    Rect label;
};

// Draws the hints where the layout put them, showing the gamepad prompt while
// the gamepad is the device in use. hovered is the index of the hint under the
// mouse.
void renderKeyHints(
    std::span<const KeyHint>                                            hints,
    const std::shared_ptr<sponge::platform::opengl::scene::BitmapFont>& font,
    const glm::mat4& projection, float windowWidth,
    std::span<const KeyHintRects> rects,
    std::optional<size_t>         hovered = std::nullopt);

// Index of the hint under the point.
std::optional<size_t> hitTestKeyHints(std::span<const KeyHintRects> rects,
                                      const glm::vec2&              position);

}  // namespace game::ui
