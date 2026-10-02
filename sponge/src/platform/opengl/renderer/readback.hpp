#pragma once

#include <cstdint>
#include <vector>

namespace sponge::platform::opengl::renderer {

// Linear float RGB, rows bottom to top as GL returns them.
struct Image {
    uint32_t           width  = 0;
    uint32_t           height = 0;
    std::vector<float> rgb;
};

// Whole level 0 of a texture as 32-bit float, so NaN, Inf and values past
// 8-bit range survive. Unlike a back buffer capture, which quantizes them.
Image readTexture(uint32_t texture);

// The back buffer as 0..1 floats. 8 bits per channel: this is what a screen
// capture sees.
Image readBackBuffer();

// Portable float map. Keeps NaN and Inf bit for bit.
bool writePfm(const Image& image, const char* path);

}  // namespace sponge::platform::opengl::renderer
