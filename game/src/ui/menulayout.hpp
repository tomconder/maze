#pragma once

#include <yoga/Yoga.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include "ui/keyhints.hpp"
#include "ui/layouttypes.hpp"
#include "ui/menufontsize.hpp"
#include "ui/tabbar.hpp"

namespace game::ui {

// Space around the panel that holds the rows.
inline constexpr float panelMargin = 10.F;

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
    // The layout of an option tab: a tab bar above everything else, and rows
    // of a fixed height that may overflow the panel. The bar is a row of the
    // two prompt icons around one node per tab label; setTabLabelSizes() gives
    // the label sizes, which only the font knows.
    bool optionTab = false;
};

// The Yoga tree of a menu with equal rows, built once and freed with the
// object. The tree spans the window: the menu above the key hint bar, which is
// a row of one node per hint. The window size is the only input to
// calculate(), besides the text sizes that setTabLabelSizes() and
// setKeyHintSizes() give.
class MenuLayout {
public:
    MenuLayout(const size_t rowCount, const MenuLayoutSpec& layoutSpec,
               const std::span<const KeyHint> hints) :
        spec(layoutSpec), hintSizes(hints.size()) {
        YGNodeStyleSetFlex(rootNode, 1.F);
        YGNodeInsertChild(windowNode, rootNode, 0);

        if (spec.optionTab) {
            tabBarNode = YGNodeNew();
            YGNodeStyleSetFlexDirection(tabBarNode, YGFlexDirectionRow);
            YGNodeStyleSetAlignItems(tabBarNode, YGAlignCenter);
            YGNodeStyleSetMargin(tabBarNode, YGEdgeLeft, panelMargin);
            // prompt, one node per label, prompt
            for (size_t i = 0; i < tabCount + 2; i++) {
                YGNodeInsertChild(tabBarNode, YGNodeNew(), static_cast<int>(i));
            }
            YGNodeInsertChild(rootNode, tabBarNode, 0);
        }

        auto* const title = YGNodeNew();
        YGNodeStyleSetFlexGrow(title, spec.titleFlexGrow);
        YGNodeInsertChild(rootNode, title,
                          static_cast<int>(YGNodeGetChildCount(rootNode)));

        YGNodeStyleSetFlex(menuNode, 1.F);
        YGNodeStyleSetFlexDirection(menuNode, YGFlexDirectionRow);
        YGNodeInsertChild(rootNode, menuNode,
                          static_cast<int>(YGNodeGetChildCount(rootNode)));

        YGNodeStyleSetMargin(backgroundNode, YGEdgeAll, panelMargin);
        YGNodeStyleSetWidthPercent(backgroundNode, spec.backgroundWidthPercent);
        YGNodeInsertChild(menuNode, backgroundNode, 0);

        for (size_t i = 0; i < rowCount; i++) {
            auto* const row = YGNodeNew();
            YGNodeInsertChild(backgroundNode, row, static_cast<int>(i));
            rows.push_back(row);
        }

        // Back and the left group, a spacer that takes the rest of the width,
        // then the right group. A hint is an icon beside its label.
        YGNodeStyleSetFlexDirection(hintBarNode, YGFlexDirectionRow);
        YGNodeStyleSetAlignItems(hintBarNode, YGAlignCenter);
        YGNodeInsertChild(windowNode, hintBarNode, 1);

        hintNodes.resize(hints.size());
        const auto order = keyHintOrder(hints);
        for (const auto index : order.left) {
            addHint(index);
        }
        auto* const spacer = YGNodeNew();
        YGNodeStyleSetFlexGrow(spacer, 1.F);
        YGNodeInsertChild(hintBarNode, spacer,
                          static_cast<int>(YGNodeGetChildCount(hintBarNode)));
        for (const auto index : order.right) {
            addHint(index);
        }
    }

    ~MenuLayout() {
        YGNodeFreeRecursive(windowNode);
    }

    MenuLayout(const MenuLayout&)            = delete;
    MenuLayout& operator=(const MenuLayout&) = delete;

    // Equal rows share the panel height, up to a maximum; the rows of an
    // option tab keep that maximum as their height. Row heights are set here,
    // not at construction, because the window width is only known once the
    // layout runs.
    void calculate(const float width, const float height) {
        if (tabBarNode != nullptr) {
            sizeTabBar(width);
        }
        for (auto* const row : rows) {
            if (spec.optionTab) {
                YGNodeStyleSetHeight(row, menuRowHeight(width));
            } else {
                YGNodeStyleSetFlex(row, 1.F);
                YGNodeStyleSetMaxHeight(row, menuRowHeight(width));
            }
        }
        sizeKeyHintBar(width);
        YGNodeStyleSetWidth(rootNode, width * spec.rootWidthScale);
        YGNodeStyleSetWidth(windowNode, width);
        YGNodeStyleSetHeight(windowNode, height);
        YGNodeCalculateLayout(windowNode, width, height, YGDirectionLTR);
    }

    // x, y, width, height of the panel that holds the rows.
    std::tuple<float, float, float, float> backgroundRect() const {
        const auto [rootX, rootY, rootW, rootH] =
            getNodeLayout(rootNode, 0.F, 0.F);
        const auto [menuX, menuY, menuW, menuH] =
            getNodeLayout(menuNode, rootX, rootY);
        return getNodeLayout(backgroundNode, menuX, menuY);
    }

    // x, y, width, height of a row in window pixels.
    std::tuple<float, float, float, float> rowRect(const size_t row) const {
        const auto [bgX, bgY, bgW, bgH] = backgroundRect();
        return getNodeLayout(rows[row], bgX, bgY);
    }

    // The whole window.
    YGNodeRef root() const {
        return windowNode;
    }

    void setTabLabelSizes(const std::array<TextSize, tabCount>& sizes) {
        labelSizes = sizes;
    }

    // One size per hint, in the order of the hints the layout was built with.
    void setKeyHintSizes(std::vector<KeyHintSize> sizes) {
        hintSizes = std::move(sizes);
    }

    // Where the nodes of every hint are, in window pixels, in the order of the
    // hints the layout was built with.
    std::vector<KeyHintRects> keyHintRects() const {
        const auto [barX, barY, barW, barH] =
            getNodeLayout(hintBarNode, 0.F, 0.F);
        const auto rectOf = [](const YGNodeRef node, const float x,
                               const float y) {
            const auto [left, top, w, h] = getNodeLayout(node, x, y);
            return Rect{ left, top, w, h };
        };

        std::vector<KeyHintRects> rects;
        for (auto* const node : hintNodes) {
            const auto hint = rectOf(node, barX, barY);
            rects.push_back(
                { hint, rectOf(YGNodeGetChild(node, 0), hint.x, hint.y),
                  rectOf(YGNodeGetChild(node, 1), hint.x, hint.y) });
        }
        return rects;
    }

    // Where the nodes of the tab bar are, in window pixels. Only for a layout
    // with optionTab set.
    TabBarRects tabBarRects() const {
        const auto [barX, barY, barW, barH] =
            getNodeLayout(tabBarNode, 0.F, 0.F);
        const auto rectOf = [&](const uint32_t child) {
            const auto [x, y, w, h] =
                getNodeLayout(YGNodeGetChild(tabBarNode, child), barX, barY);
            return Rect{ x, y, w, h };
        };

        TabBarRects rects{};
        rects.prev = rectOf(0);
        for (uint32_t i = 0; i < tabCount; i++) {
            rects.labels[i] = rectOf(i + 1);
        }
        rects.next = rectOf(tabCount + 1);
        return rects;
    }

    size_t rowCount() const {
        return rows.size();
    }

private:
    void addHint(const size_t index) {
        auto* const hint = YGNodeNew();
        YGNodeStyleSetFlexDirection(hint, YGFlexDirectionRow);
        YGNodeStyleSetAlignItems(hint, YGAlignCenter);
        YGNodeInsertChild(hint, YGNodeNew(), 0);  // icon
        YGNodeInsertChild(hint, YGNodeNew(), 1);  // label
        YGNodeInsertChild(hintBarNode, hint,
                          static_cast<int>(YGNodeGetChildCount(hintBarNode)));
        hintNodes[index] = hint;
    }

    void sizeKeyHintBar(const float width) {
        YGNodeStyleSetHeight(hintBarNode, keyHintBarHeight(width));
        YGNodeStyleSetPadding(hintBarNode, YGEdgeHorizontal,
                              keyHintMarginX(width));
        YGNodeStyleSetGap(hintBarNode, YGGutterColumn, keyHintGap(width) * 2.F);

        for (size_t i = 0; i < hintNodes.size(); i++) {
            auto* const node = hintNodes[i];
            YGNodeStyleSetGap(node, YGGutterColumn, keyHintGap(width));

            auto* const icon = YGNodeGetChild(node, 0);
            YGNodeStyleSetWidth(icon, hintSizes[i].icon);
            YGNodeStyleSetHeight(icon, hintSizes[i].icon);

            auto* const label = YGNodeGetChild(node, 1);
            YGNodeStyleSetWidth(label, hintSizes[i].label.width);
            YGNodeStyleSetHeight(label, hintSizes[i].label.height);
        }
    }

    void sizeTabBar(const float width) {
        YGNodeStyleSetHeight(tabBarNode, tabBarHeight(width));
        YGNodeStyleSetGap(tabBarNode, YGGutterColumn, tabGap(width));

        const auto icon = tabIconSize(width);
        for (const auto child : { 0U, static_cast<uint32_t>(tabCount) + 1 }) {
            auto* const node = YGNodeGetChild(tabBarNode, child);
            YGNodeStyleSetWidth(node, icon);
            YGNodeStyleSetHeight(node, icon);
        }
        for (uint32_t i = 0; i < tabCount; i++) {
            auto* const node = YGNodeGetChild(tabBarNode, i + 1);
            YGNodeStyleSetWidth(node, labelSizes[i].width);
            YGNodeStyleSetHeight(node, labelSizes[i].height);
        }
    }

    MenuLayoutSpec         spec;
    YGNodeRef              windowNode     = YGNodeNew();
    YGNodeRef              rootNode       = YGNodeNew();
    YGNodeRef              hintBarNode    = YGNodeNew();
    YGNodeRef              tabBarNode     = nullptr;
    YGNodeRef              menuNode       = YGNodeNew();
    YGNodeRef              backgroundNode = YGNodeNew();
    std::vector<YGNodeRef> rows;

    std::array<TextSize, tabCount> labelSizes{};
    std::vector<YGNodeRef>         hintNodes;  // by hint index
    std::vector<KeyHintSize>       hintSizes;
};

}  // namespace game::ui
