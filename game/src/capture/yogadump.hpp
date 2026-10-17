#pragma once

#include <yoga/Yoga.h>

#include <string>

namespace game::capture {

// Writes a Yoga tree as JSX for the yogalayout.dev playground. Only style
// values the tree sets are written.
std::string yogaToPlayground(YGNodeRef root);

}  // namespace game::capture
