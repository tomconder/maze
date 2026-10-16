#pragma once

#include <yoga/Yoga.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <tuple>
#include <vector>

#include "ui/keyhints.hpp"
#include "ui/menufontsize.hpp"

namespace game::ui {

// Rows track the menu font, which steps with the window width.
inline float menuRowHeight(const float windowWidth) {
    return static_cast<float>(
               menuFontSizeForWidth(static_cast<uint32_t>(windowWidth))) *
           3.4F;
}

inline std::tuple<float, float, float, float>
    getNodeLayout(const YGNodeRef node, const float offsetX,
                  const float offsetY) {
    return { offsetX + YGNodeLayoutGetLeft(node),
             offsetY + YGNodeLayoutGetTop(node), YGNodeLayoutGetWidth(node),
             YGNodeLayoutGetHeight(node) };
}

struct MenuLayoutSpec {
    float backgroundWidthPercent;
    // The empty space above the rows, relative to the row area (which is 1.0).
    // Menus with many rows want a smaller share.
    float titleFlexGrow = 0.9F;
    // The share of the window width the tree spans; the exit panel is narrower
    // than the window.
    float rootWidthScale = 1.F;
};

// The Yoga tree of a menu with equal rows, built once and freed with the
// object. The window size is the only input to calculate().
class MenuLayout {
public:
    MenuLayout(const size_t rowCount, const MenuLayoutSpec& layoutSpec) :
        spec(layoutSpec) {
        auto* const title = YGNodeNew();
        YGNodeStyleSetFlexGrow(title, spec.titleFlexGrow);
        YGNodeInsertChild(rootNode, title, 0);

        YGNodeStyleSetFlex(menuNode, 1.F);
        YGNodeStyleSetFlexDirection(menuNode, YGFlexDirectionRow);
        YGNodeInsertChild(rootNode, menuNode, 1);

        YGNodeStyleSetMargin(backgroundNode, YGEdgeAll, 10.F);
        YGNodeStyleSetWidthPercent(backgroundNode, spec.backgroundWidthPercent);
        YGNodeInsertChild(menuNode, backgroundNode, 0);

        for (size_t i = 0; i < rowCount; i++) {
            auto* const row = YGNodeNew();
            YGNodeStyleSetFlex(row, 1.F);
            YGNodeInsertChild(backgroundNode, row, static_cast<int>(i));
            rows.push_back(row);
        }
    }

    ~MenuLayout() {
        YGNodeFreeRecursive(rootNode);
    }

    MenuLayout(const MenuLayout&)            = delete;
    MenuLayout& operator=(const MenuLayout&) = delete;

    // Leaves room for the key hint bar along the bottom. Row heights are set
    // here, not at construction, because the window width is only known once
    // the layout runs.
    void calculate(const float width, const float height) {
        const auto panelWidth   = width * spec.rootWidthScale;
        const auto usableHeight = heightWithoutKeyHints(width, height);
        for (auto* const row : rows) {
            YGNodeStyleSetMaxHeight(row, menuRowHeight(width));
        }
        YGNodeStyleSetWidth(rootNode, panelWidth);
        YGNodeStyleSetHeight(rootNode, usableHeight);
        YGNodeCalculateLayout(rootNode, panelWidth, usableHeight,
                              YGDirectionLTR);
    }

    // x, y, width, height of the panel that holds the rows. originY is the
    // height reserved above the tree, such as a tab bar.
    std::tuple<float, float, float, float>
        backgroundRect(const float originY = 0.F) const {
        const auto [rootX, rootY, rootW, rootH] =
            getNodeLayout(rootNode, 0.F, originY);
        const auto [menuX, menuY, menuW, menuH] =
            getNodeLayout(menuNode, rootX, rootY);
        return getNodeLayout(backgroundNode, menuX, menuY);
    }

    // x, y, width, height of a row in window pixels.
    std::tuple<float, float, float, float>
        rowRect(const size_t row, const float originY = 0.F) const {
        const auto [bgX, bgY, bgW, bgH] = backgroundRect(originY);
        return getNodeLayout(rows[row], bgX, bgY);
    }

    YGNodeRef root() const {
        return rootNode;
    }

    std::span<const YGNodeRef> rowNodes() const {
        return rows;
    }

private:
    MenuLayoutSpec         spec;
    YGNodeRef              rootNode       = YGNodeNew();
    YGNodeRef              menuNode       = YGNodeNew();
    YGNodeRef              backgroundNode = YGNodeNew();
    std::vector<YGNodeRef> rows;
};

}  // namespace game::ui
