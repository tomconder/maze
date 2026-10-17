#pragma once

#include <cstdint>
#include <string_view>

namespace game::capture {

// Writes the Yoga tree of a menu screen as playground JSX to
// capture_menu_<screen>.jsx, laid out at the window size. The tree comes from
// the same functions the layers use to build their rows, so it needs no window
// and no OpenGL, only the baked font. The display tab lists three aspect
// ratios and four resolutions in place of what a monitor reports. Returns
// false for a name that is not a screen: intro, exit, display, keyboard,
// audio, confirm or reset.
bool writeMenuYoga(std::string_view screen, uint32_t width, uint32_t height);

}  // namespace game::capture
