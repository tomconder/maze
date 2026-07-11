#pragma once

#include <yoga/Yoga.h>

#include <tuple>

namespace game::ui {

struct MenuSkeleton {
    YGNodeRef root;
    YGNodeRef menu;
    YGNodeRef menuBackground;
};

inline MenuSkeleton buildMenuSkeleton(const float menuBackgroundWidthPercent) {
    auto* const root = YGNodeNew();

    auto* const title = YGNodeNew();
    YGNodeStyleSetFlexGrow(title, 0.9F);
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
