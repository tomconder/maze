#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace assetconv {

// How the channels of a mip level are averaged.
enum class MipSpace : uint8_t {
    // sRGB colour: filtered in linear light and weighted by alpha, so
    // transparent texels add no colour. Alpha is filtered as it is.
    Srgb,
    // Every channel is data, alpha included: no decode, no weighting.
    Linear,
};

// Halves an RGBA8 image to max(width / 2, 1) x max(height / 2, 1) with a
// Mitchell filter (B = C = 1/3) stretched to the scale ratio, so odd sizes
// need no special case. Edge texels repeat past the border.
std::vector<uint8_t> halveMip(std::span<const uint8_t> pixels, uint32_t width,
                              uint32_t height, MipSpace space);

}  // namespace assetconv
