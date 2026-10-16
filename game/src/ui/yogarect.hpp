#pragma once

#include <yoga/Yoga.h>

#include "ui/layouttypes.hpp"

namespace game::ui {

// Where a node is in window pixels, once the tree around it has been laid out:
// the offsets of the node and all its parents, and the size of the node.
inline Rect nodeRect(const YGNodeRef node) {
    float x = 0.F;
    float y = 0.F;
    for (auto* n = node; n != nullptr; n = YGNodeGetParent(n)) {
        x += YGNodeLayoutGetLeft(n);
        y += YGNodeLayoutGetTop(n);
    }
    return { x, y, YGNodeLayoutGetWidth(node), YGNodeLayoutGetHeight(node) };
}

}  // namespace game::ui
