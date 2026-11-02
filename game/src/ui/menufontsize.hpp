#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace game::ui {

// Sizes baked into the font. Keep in step with `sizes` in assets/manifest.yaml.
inline constexpr std::array<uint32_t, 11> bakedFontSizes = { 12, 14, 16, 18,
                                                             21, 24, 27, 32,
                                                             43, 48, 56 };

// The menu font is 21 px at 1920 wide and scales with the window width, so
// every 16:9 size shows the same picture. Rows, tab bar and key hints derive
// from this size. The font holds only baked sizes, so the result snaps to the
// nearest one; sizes between steps need the font renderer to scale a baked
// size.
inline uint32_t menuFontSizeForWidth(const uint32_t windowWidth,
                                     const float    sizeAt1920 = 21.F) {
    const auto target = sizeAt1920 * static_cast<float>(windowWidth) / 1920.F;
    auto       best   = bakedFontSizes.front();
    for (const auto size : bakedFontSizes) {
        if (std::abs(static_cast<float>(size) - target) <
            std::abs(static_cast<float>(best) - target)) {
            best = size;
        }
    }
    return best;
}

}  // namespace game::ui
