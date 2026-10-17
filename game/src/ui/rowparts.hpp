#pragma once

#include <yoga/Yoga.h>

#include "ui/layouttypes.hpp"
#include "ui/yogarect.hpp"

namespace game::ui {

// The label is the first part of every row.
inline constexpr int labelPart = 0;

// A widget in a row of a menu is a run of parts, left to right. The first part
// is the label and takes the width the others leave; the row keeps a margin on
// both sides.
inline void attachRowParts(const YGNodeRef row, const int count,
                           const float margin) {
    YGNodeStyleSetFlexDirection(row, YGFlexDirectionRow);
    YGNodeStyleSetAlignItems(row, YGAlignCenter);
    YGNodeStyleSetPadding(row, YGEdgeHorizontal, margin);
    for (int part = 0; part < count; part++) {
        YGNodeInsertChild(row, YGNodeNew(), part);
    }
    YGNodeStyleSetFlexGrow(YGNodeGetChild(row, labelPart), 1.F);
}

// width may be YGUndefined, for a part that takes what the rest leaves.
inline void setPartSize(const YGNodeRef row, const int part, const float width,
                        const float height) {
    auto* const node = YGNodeGetChild(row, part);
    YGNodeStyleSetWidth(node, width);
    YGNodeStyleSetHeight(node, height);
}

// Where a node under the row is in window pixels, moved so the row starts at
// y: the rows scroll, and the nodes stay where Yoga put them.
inline Rect rectAt(const YGNodeRef row, const YGNodeRef node, const float y) {
    auto rect = nodeRect(node);
    rect.y += y - nodeRect(row).y;
    return rect;
}

inline Rect partRect(const YGNodeRef row, const int part, const float y) {
    return rectAt(row, YGNodeGetChild(row, part), y);
}

}  // namespace game::ui
