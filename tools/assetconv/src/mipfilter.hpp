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

// The alpha test of a glTF MASK material keeps a texel when alpha >= cutoff.
// Averaging thins a mip's alpha, so fewer texels pass at each level and the
// foliage fades away with distance. These two keep the share of passing
// texels the same at every level.
//
// threshold is the cutoff as an 8-bit alpha value, 1 to 255: the smallest
// stored alpha that passes.

// Fraction of texels, 0 to 1, that pass the alpha test.
double alphaCoverage(std::span<const uint8_t> pixels, uint8_t threshold);

// Scales the alpha of a mip so that the fraction passing the test is as close
// to target as 8 bits allow, never below it. Colour is left alone. Does
// nothing when target is 0 or 1: nothing, or everything, passes at every
// scale.
void scaleAlphaCoverage(std::span<uint8_t> pixels, uint8_t threshold,
                        double target);

}  // namespace assetconv
