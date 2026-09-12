#pragma once

#include "scene/modeldata.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace assetconv {

// What a texture is for. Decides the block format and whether the mip chain
// is filtered in sRGB or linear space.
enum class TextureKind : uint8_t {
    // Albedo and emissive: three or four channels through BC7.
    Color,
    // Normal maps: XY through BC5, Z reconstructed in the shader.
    Normal,
    // Occlusion and metallic-roughness: BC7, but resampled linearly.
    Linear,
};

// Must run once before any encode.
void initEncoder();

// Decodes an image file to RGBA8. Returns an image with zero width on
// failure. Shared by the single-texture and atlas paths.
sponge::scene::ParsedImage loadImage(const std::string& path);

// Compresses an image and returns the whole KTX2 file, mip chain included.
// Returns an empty vector if the image is unusable.
std::vector<uint8_t> encode(const sponge::scene::ParsedImage& image,
                            TextureKind                       kind);

}  // namespace assetconv
