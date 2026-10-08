#pragma once

#include "modeldata.hpp"

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

// Must run once before any encode. threads is the number of threads each
// image is split across, and the number glTF image decoding and mip filtering
// use; 0 means one per hardware thread.
void initEncoder(unsigned threads);

// Decodes an image file to RGBA8. Returns an image with zero width on
// failure.
sponge::scene::ParsedImage loadImage(const std::string& path);

// Compresses an image and returns the whole KTX2 file, mip chain included.
// Returns an empty vector if the image is unusable.
//
// alphaCutoff is the alpha a texel needs to pass a glTF MASK test, 0 for none.
// Each mip then has its alpha scaled so the share of passing texels matches
// level 0, which keeps foliage from thinning with distance. Colour only.
std::vector<uint8_t> encode(const sponge::scene::ParsedImage& image,
                            TextureKind kind, float alphaCutoff = 0.F);

}  // namespace assetconv
