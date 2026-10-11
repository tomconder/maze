#pragma once

#include <yoga/Yoga.h>

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>

#include "platform/opengl/renderer/rendererapi.hpp"
#include "platform/opengl/scene/quad.hpp"
#include "ui/menulayout.hpp"

namespace game::ui {

// Scrolling for a menu with more rows than fit. The target and the drawn
// position (offset) are both in rows and can fall between two rows, so a row
// can rest half in view. The offset eases toward the target. Callers skip rows
// outside the window (see shows()), draw the rest between beginClip() and
// endClip(), and test mouse hits with canHit().
//
// Only the first `count` rows scroll. The rows after them stay pinned at the
// bottom, and shows() is always true for those.
class MenuScroll {
public:
    constexpr void setWindow(const size_t rowCount, const size_t fit) {
        count   = rowCount;
        visible = std::min(std::max<size_t>(fit, 1), rowCount);
        setTarget(target);
        offset = std::min(offset, maxOffset());
    }

    constexpr bool overflows() const {
        return count > visible;
    }

    // True for a row that is at least partly inside the window.
    constexpr bool shows(const size_t row) const {
        const auto top = static_cast<float>(row);
        return row >= count || (top + 1.F > offset &&
                                top < offset + static_cast<float>(visible));
    }

    // True when a mouse at height y is over the row and the row is in view.
    constexpr bool canHit(const size_t row, const float y) const {
        return row >= count ||
               (shows(row) && y >= trackY && y <= trackY + trackH);
    }

    // Jumps the drawn position to the target, with no easing.
    constexpr void snap() {
        offset = target;
    }

    // Eases the drawn position toward the target. Returns true while it moves,
    // so the caller lays out again. Seconds since the last frame.
    constexpr bool step(const float seconds) {
        const float diff = target - offset;
        if (diff < 0.002F && diff > -0.002F) {
            const bool moved = diff != 0.F;
            snap();
            return moved;
        }
        offset += diff * std::min(1.F, seconds * scrollSpeed);
        return true;
    }

    // Each returns true when the target moved, so the caller lays out again.
    constexpr bool scrollBy(const float rows) {
        return setTarget(target + rows);
    }

    // Moves the target just far enough to show the whole row.
    constexpr bool ensureVisible(const size_t row) {
        if (row >= count) {
            return false;
        }
        const auto top = static_cast<float>(row);
        if (top < target) {
            return setTarget(top);
        }
        return setTarget(
            std::max(target, top + 1.F - static_cast<float>(visible)));
    }

    // yOffset is positive when the wheel turns up.
    constexpr bool wheel(const float yOffset) {
        return scrollBy(-yOffset * wheelRows);
    }

    // Fits the rows to the space above the pinned footer and lays out. rows are
    // the scrolling rows in order. footer is the first pinned row. originY is
    // the height reserved above the root.
    void layout(const YGNodeRef root, const YGNodeRef menu,
                const YGNodeRef                  background,
                const std::span<const YGNodeRef> rows, const YGNodeRef footer,
                const float width, const float height, const float originY) {
        const float rowHeight = menuRowHeight(width);
        for (size_t i = 0; i < rows.size(); i++) {
            // out of the flex flow, so the pinned rows keep their place
            YGNodeStyleSetPositionType(rows[i], YGPositionTypeAbsolute);
            YGNodeStyleSetPosition(rows[i], YGEdgeLeft, 0.F);
            YGNodeStyleSetWidthPercent(rows[i], 100.F);
            YGNodeStyleSetHeight(rows[i], rowHeight);
        }
        YGNodeStyleSetWidth(root, width);
        YGNodeStyleSetHeight(root, height);
        YGNodeCalculateLayout(root, width, height, YGDirectionLTR);

        setWindow(rows.size(),
                  static_cast<size_t>(YGNodeLayoutGetTop(footer) / rowHeight +
                                      0.001F));
        for (size_t i = 0; i < rows.size(); i++) {
            YGNodeStyleSetPosition(rows[i], YGEdgeTop,
                                   (static_cast<float>(i) - offset) *
                                       rowHeight);
        }
        YGNodeCalculateLayout(root, width, height, YGDirectionLTR);

        const auto [rootX, rootY, rootW, rootH] =
            getNodeLayout(root, 0.F, originY);
        const auto [menuX, menuY, menuW, menuH] =
            getNodeLayout(menu, rootX, rootY);
        const auto [bgX, bgY, bgW, bgH] =
            getNodeLayout(background, menuX, menuY);
        thickness = std::max(6.F, std::round(rowHeight / 20.F));
        trackX    = bgX + bgW + thickness;
        trackY    = bgY;
        trackH    = static_cast<float>(visible) * rowHeight;
        clipW     = width;
    }

    // Limits drawing to the window of rows, so a half shown row is cut off at
    // the edge. windowHeight is the height of the ortho camera.
    void beginClip(const float windowHeight) const {
        if (overflows()) {
            // GL counts from the bottom edge
            sponge::platform::opengl::renderer::RendererAPI::setScissor(
                0,
                static_cast<int32_t>(
                    std::round(windowHeight - trackY - trackH)),
                static_cast<int32_t>(clipW),
                static_cast<int32_t>(std::round(trackH)));
        }
    }

    void endClip() const {
        if (overflows()) {
            sponge::platform::opengl::renderer::RendererAPI::disableScissor();
        }
    }

    // A press on the bar grabs it and jumps to the mouse.
    bool press(const glm::vec2& mouse) {
        if (!overflows() || mouse.x < trackX - thickness ||
            mouse.x > trackX + thickness * 2.F || mouse.y < trackY ||
            mouse.y > trackY + trackH) {
            return false;
        }
        dragging = true;
        dragTo(mouse.y);
        return true;
    }

    bool isDragging() const {
        return dragging;
    }

    // Returns true when the window moved.
    bool drag(const float mouseY) {
        return dragging && dragTo(mouseY);
    }

    void release() {
        dragging = false;
    }

    void render(const sponge::platform::opengl::scene::Quad& quad) const {
        if (!overflows()) {
            return;
        }
        const float radius = thickness / 2.F;
        quad.render({ trackX, trackY }, { trackX + thickness, trackY + trackH },
                    trackColor, radius);

        const float thumbH = thumbHeight();
        const float thumbY = trackY + (trackH - thumbH) * offset /
                                          static_cast<float>(count - visible);
        quad.render({ trackX, thumbY }, { trackX + thickness, thumbY + thumbH },
                    dragging ? thumbDragColor : thumbColor, radius);
    }

private:
    static constexpr glm::vec4 trackColor{ 0.84F, 0.84F, 0.84F, 0.14F };
    static constexpr glm::vec4 thumbColor{ 0.84F, 0.84F, 0.84F, 0.5F };
    static constexpr glm::vec4 thumbDragColor{ 1.F, 1.F, 1.F, 0.8F };

    size_t count   = 0;
    size_t visible = 0;

    // Rows per second of easing; a larger value catches up faster.
    static constexpr float scrollSpeed = 18.F;

    // Rows moved by one wheel notch.
    static constexpr float wheelRows = 0.5F;

    // Both in rows from the top. The drawn offset eases toward the target.
    float target = 0.F;
    float offset = 0.F;

    float clipW     = 0.F;
    float trackX    = 0.F;
    float trackY    = 0.F;
    float trackH    = 0.F;
    float thickness = 0.F;

    bool dragging = false;

    constexpr float maxOffset() const {
        return static_cast<float>(count - visible);
    }

    constexpr bool setTarget(const float rows) {
        const float next  = std::clamp(rows, 0.F, maxOffset());
        const bool  moved = next != target;
        target            = next;
        return moved;
    }

    constexpr float thumbHeight() const {
        return std::min(
            std::max(thickness * 2.F, trackH * static_cast<float>(visible) /
                                          static_cast<float>(count)),
            trackH * 0.9F);
    }

    // The thumb centres on the mouse.
    bool dragTo(const float mouseY) {
        const float thumbH = thumbHeight();
        const float t      = std::clamp(
            (mouseY - trackY - thumbH / 2.F) / (trackH - thumbH), 0.F, 1.F);
        return setTarget(t * maxOffset());
    }
};

constexpr bool menuScrollWorks() {
    MenuScroll scroll;
    scroll.setWindow(10, 4);
    bool ok = scroll.ensureVisible(5) && !scroll.ensureVisible(3);
    scroll.snap();  // target is 2 rows: rows 2 to 5 fill the window
    ok = ok && scroll.shows(2) && !scroll.shows(1) && !scroll.shows(6);
    ok = ok && scroll.wheel(-1.F);  // half a row down: row 6 peeks in
    scroll.snap();
    ok = ok && scroll.shows(6) && scroll.shows(2) && !scroll.shows(1);
    ok = ok && scroll.ensureVisible(0) && scroll.scrollBy(100.F);
    scroll.snap();
    ok = ok && scroll.shows(6) && !scroll.shows(5) && scroll.shows(10);
    scroll.scrollBy(-0.5F);
    ok = ok && scroll.step(0.01F) && scroll.shows(5) && scroll.shows(9);
    scroll.setWindow(10, 12);
    return ok && !scroll.overflows() && scroll.shows(10);
}
static_assert(menuScrollWorks());

}  // namespace game::ui
