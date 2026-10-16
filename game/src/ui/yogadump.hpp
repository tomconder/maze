#pragma once

#include <yoga/Yoga.h>

#include <string>

namespace game::ui {

// Writes a Yoga tree as JSX for the yogalayout.dev playground. Only style
// values the tree sets are written. The tree sits in a window-sized column
// between an empty node for the tab bar above (omitted when its height is 0)
// and one for the key hint bar below, which take the rest of the window height.
std::string yogaToPlayground(YGNodeRef root, float windowWidth,
                             float windowHeight, float tabBarHeight);

}  // namespace game::ui
