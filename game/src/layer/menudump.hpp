#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "ui/layouttypes.hpp"

namespace game::layer {

// The Yoga tree of a menu screen as playground JSX, laid out at the window
// size. The tree comes from the same functions the layers use to build their
// rows, so it needs no window and no OpenGL, only a text measure. The display
// tab lists three aspect ratios and four resolutions in place of what a
// monitor reports. Returns nothing for a name that is not a screen: intro,
// exit, display, keyboard, audio, confirm or reset.
std::optional<std::string> dumpMenuYoga(std::string_view screen, uint32_t width,
                                        uint32_t               height,
                                        const ui::TextMeasure& measure);

}  // namespace game::layer
