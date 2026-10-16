#pragma once

#include <yoga/Yoga.h>

#include <cstdint>
#include <optional>

#include "ui/layouttypes.hpp"
#include "ui/yogarect.hpp"

namespace game::ui {

// The Yoga tree of a confirm dialog: a panel centred in the window, holding a
// title, a message, an optional countdown line and a row of two buttons. The
// panel is as wide as its widest text. Built once and freed with the object;
// calculate() takes the window size, the font size and the text sizes, which
// only the font knows.
class ConfirmLayout {
public:
    ConfirmLayout() {
        YGNodeStyleSetJustifyContent(windowNode, YGJustifyCenter);
        YGNodeStyleSetAlignItems(windowNode, YGAlignCenter);
        YGNodeStyleSetAlignItems(panelNode, YGAlignCenter);
        YGNodeInsertChild(windowNode, panelNode, 0);

        YGNodeStyleSetFlexDirection(buttonRowNode, YGFlexDirectionRow);
        YGNodeStyleSetAlignSelf(buttonRowNode, YGAlignStretch);
        YGNodeStyleSetFlex(acceptNode, 1.F);
        YGNodeStyleSetFlex(cancelNode, 1.F);
        YGNodeInsertChild(buttonRowNode, acceptNode, 0);
        YGNodeInsertChild(buttonRowNode, cancelNode, 1);

        int index = 0;
        for (auto* const node :
             { titleNode, messageNode, countdownNode, buttonRowNode }) {
            YGNodeInsertChild(panelNode, node, index++);
        }
    }

    ~ConfirmLayout() {
        YGNodeFreeRecursive(windowNode);
    }

    ConfirmLayout(const ConfirmLayout&)            = delete;
    ConfirmLayout& operator=(const ConfirmLayout&) = delete;

    // The countdown line is left out of the layout without a countdown size.
    void calculate(const float width, const float height,
                   const uint32_t fontSize, const TextSize& title,
                   const TextSize&                message,
                   const std::optional<TextSize>& countdown) {
        const auto size = static_cast<float>(fontSize);
        const auto pad  = size * 1.25F;
        const auto gap  = size * 0.5F;

        YGNodeStyleSetWidth(windowNode, width);
        YGNodeStyleSetHeight(windowNode, height);
        YGNodeStyleSetPadding(panelNode, YGEdgeAll, pad);
        YGNodeStyleSetGap(panelNode, YGGutterRow, gap);
        YGNodeStyleSetGap(buttonRowNode, YGGutterColumn, gap);
        YGNodeStyleSetHeight(buttonRowNode, size * 2.2F);

        setSize(titleNode, title);
        setSize(messageNode, message);
        YGNodeStyleSetDisplay(countdownNode,
                              countdown ? YGDisplayFlex : YGDisplayNone);
        setSize(countdownNode, countdown.value_or(TextSize{ 0.F, 0.F }));

        YGNodeCalculateLayout(windowNode, width, height, YGDirectionLTR);
    }

    // The whole window.
    YGNodeRef root() const {
        return windowNode;
    }

private:
    static void setSize(const YGNodeRef node, const TextSize& size) {
        YGNodeStyleSetWidth(node, size.width);
        YGNodeStyleSetHeight(node, size.height);
    }

public:
    // Read where they ended up with nodeRect().
    YGNodeRef windowNode    = YGNodeNew();
    YGNodeRef panelNode     = YGNodeNew();
    YGNodeRef titleNode     = YGNodeNew();
    YGNodeRef messageNode   = YGNodeNew();
    YGNodeRef countdownNode = YGNodeNew();
    YGNodeRef buttonRowNode = YGNodeNew();
    YGNodeRef acceptNode    = YGNodeNew();
    YGNodeRef cancelNode    = YGNodeNew();
};

}  // namespace game::ui
