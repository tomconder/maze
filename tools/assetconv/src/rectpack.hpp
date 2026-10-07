#pragma once

#include <finders_interface.h>

#include <vector>

namespace assetconv {

// Packs the rects into the smallest bin of at most maxSide x maxSide and
// writes each rect's x and y in place; the order of `rects` does not change.
// Returns the bounding size of the packing, or 0x0 when a rect does not fit.
// Rects never rotate: sprites and glyphs are sampled axis-aligned.
inline rectpack2D::rect_wh packRects(std::vector<rectpack2D::rect_xywh>& rects,
                                     const int maxSide) {
    using spaces = rectpack2D::empty_spaces<false>;

    bool       failed = false;
    const auto size   = rectpack2D::find_best_packing<spaces>(
        rects,
        rectpack2D::make_finder_input(
            maxSide, 1,
            [](auto&) { return rectpack2D::callback_result::CONTINUE_PACKING; },
            [&failed](auto&) {
                failed = true;
                return rectpack2D::callback_result::ABORT_PACKING;
            },
            rectpack2D::flipping_option::DISABLED));
    return failed ? rectpack2D::rect_wh{} : size;
}

}  // namespace assetconv
