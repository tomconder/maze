#pragma once

#include <yoga/Yoga.h>

#include <tuple>

#include "ui/menufontsize.hpp"

namespace game::ui {

struct MenuSkeleton {
    YGNodeRef root;
    YGNodeRef menu;
    YGNodeRef menuBackground;
};

// titleFlexGrow sets the empty space above the rows, relative to the row area
// (which is 1.0). Menus with many rows want a smaller share.
inline MenuSkeleton buildMenuSkeleton(const float menuBackgroundWidthPercent,
                                      const float titleFlexGrow = 0.9F) {
    auto* const root = YGNodeNew();

    auto* const title = YGNodeNew();
    YGNodeStyleSetFlexGrow(title, titleFlexGrow);
    YGNodeInsertChild(root, title, 0);

    auto* const menu = YGNodeNew();
    YGNodeStyleSetFlex(menu, 1.F);
    YGNodeStyleSetFlexDirection(menu, YGFlexDirectionRow);
    YGNodeInsertChild(root, menu, 1);

    auto* const menuBackground = YGNodeNew();
    YGNodeStyleSetMargin(menuBackground, YGEdgeAll, 10.F);
    YGNodeStyleSetWidthPercent(menuBackground, menuBackgroundWidthPercent);
    YGNodeInsertChild(menu, menuBackground, 0);

    return { root, menu, menuBackground };
}

// Rows track the menu font, which steps with the window width.
inline float menuRowHeight(const float windowWidth) {
    return static_cast<float>(
               menuFontSizeForWidth(static_cast<uint32_t>(windowWidth))) *
           3.4F;
}

// Auto margin takes the leftover space, the fixed height keeps the row off the
// flex share, so it lands in the same place whatever sits above it.
// Row heights are set here, not at construction, because the window width is
// only known once the layout runs.
inline void setMenuRowHeight(const YGNodeRef row, const float windowWidth) {
    YGNodeStyleSetMaxHeight(row, menuRowHeight(windowWidth));
}

inline void pinMenuRowToBottom(const YGNodeRef row, const float windowWidth) {
    YGNodeStyleSetMarginAuto(row, YGEdgeTop);
    YGNodeStyleSetFlexGrow(row, 0.F);
    YGNodeStyleSetFlexShrink(row, 0.F);
    // makeMenuRow left a flex basis of 0, so the siblings would grow into
    // this row's space; the basis is what reserves it
    YGNodeStyleSetFlexBasis(row, menuRowHeight(windowWidth));
    YGNodeStyleSetHeight(row, menuRowHeight(windowWidth));
}

inline YGNodeRef makeMenuRow(const YGNodeRef parent, const int index) {
    auto* const child = YGNodeNew();
    YGNodeStyleSetFlex(child, 1.F);
    YGNodeStyleSetMaxHeight(child, 110);
    YGNodeInsertChild(parent, child, index);
    return child;
}

inline std::tuple<float, float, float, float>
    getNodeLayout(const YGNodeRef node, const float offsetX,
                  const float offsetY) {
    return { offsetX + YGNodeLayoutGetLeft(node),
             offsetY + YGNodeLayoutGetTop(node), YGNodeLayoutGetWidth(node),
             YGNodeLayoutGetHeight(node) };
}

}  // namespace game::ui
