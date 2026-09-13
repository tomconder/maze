#include "texenc.hpp"

#include "ktx2.hpp"
#include "modeldata.hpp"

#include <fmt/base.h>
#include <bc7enc.h>
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {
namespace ktx2 = sponge::scene::ktx2;

constexpr uint32_t blockSize     = 4;
constexpr size_t   texelsInBlock = blockSize * blockSize;
constexpr size_t   bc7BlockBytes = 16;
constexpr size_t   bc4BlockBytes = 8;
constexpr size_t   rgbaChannels  = 4;

// One level of an RGBA8 image.
struct Surface {
    uint32_t             width{ 0 };
    uint32_t             height{ 0 };
    std::vector<uint8_t> pixels;  // RGBA8, tightly packed
};

// BC7 always wants four channels, and a 1- or 3-channel source would
// otherwise need a separate path per channel count.
Surface toRgba(const sponge::scene::ParsedImage& image) {
    Surface surface{ .width  = image.width,
                     .height = image.height,
                     .pixels =
                         std::vector<uint8_t>(static_cast<size_t>(image.width) *
                                              image.height * rgbaChannels) };

    const auto channels = image.bytesPerPixel;
    for (size_t i = 0; i < static_cast<size_t>(image.width) * image.height;
         i++) {
        const auto* src = &image.pixels[i * channels];
        auto*       dst = &surface.pixels[i * rgbaChannels];
        dst[0]          = src[0];
        dst[1]          = channels > 1 ? src[1] : src[0];
        dst[2]          = channels > 2 ? src[2] : src[0];
        dst[3]          = channels > 3 ? src[3] : 255;
    }
    return surface;
}

// Copies one 4x4 block out of a surface. Blocks that hang off the right or
// bottom edge repeat the last row and column rather than reading past the
// buffer; the decoder never samples those texels.
std::array<uint8_t, texelsInBlock * rgbaChannels>
    readBlock(const Surface& surface, const uint32_t blockX,
              const uint32_t blockY) {
    std::array<uint8_t, texelsInBlock * rgbaChannels> block{};
    for (uint32_t y = 0; y < blockSize; y++) {
        const auto sourceY =
            std::min(blockY + y, surface.height - 1) * surface.width;
        for (uint32_t x = 0; x < blockSize; x++) {
            const auto  sourceX = std::min(blockX + x, surface.width - 1);
            const auto* src =
                &surface.pixels[(sourceY + sourceX) * rgbaChannels];
            auto* dst = &block[((y * blockSize) + x) * rgbaChannels];
            std::copy_n(src, rgbaChannels, dst);
        }
    }
    return block;
}

// One BC4 block: two endpoints plus sixteen 3-bit indices. Endpoint order
// red0 > red1 selects the eight-value ramp, so index 0 decodes to red0,
// index 1 to red1, and 2..7 to the six interpolated steps between them.
//
// Indices come from a nearest search over the decoded ramp instead of an
// arithmetic mapping. Eight comparisons per texel costs nothing here and
// there is no index order to get wrong.
void encodeBc4(const std::array<uint8_t, texelsInBlock * rgbaChannels>& block,
               const size_t channel, uint8_t* out) {
    std::array<uint8_t, texelsInBlock> values{};
    for (size_t i = 0; i < texelsInBlock; i++) {
        values[i] = block[(i * rgbaChannels) + channel];
    }

    const auto [minIt, maxIt] = std::ranges::minmax_element(values);
    const auto low            = *minIt;
    const auto high           = *maxIt;

    out[0] = high;
    out[1] = low;

    std::array<int32_t, 8> ramp{};
    ramp[0] = high;
    ramp[1] = low;
    for (size_t i = 2; i < ramp.size(); i++) {
        ramp[i] = (((8 - static_cast<int32_t>(i)) * high) +
                   ((static_cast<int32_t>(i) - 1) * low)) /
                  7;
    }

    uint64_t indices = 0;
    for (size_t i = 0; i < texelsInBlock; i++) {
        size_t  best      = 0;
        int32_t bestError = INT32_MAX;
        for (size_t candidate = 0; candidate < ramp.size(); candidate++) {
            const auto error = std::abs(ramp[candidate] - values[i]);
            if (error < bestError) {
                bestError = error;
                best      = candidate;
            }
        }
        indices |= static_cast<uint64_t>(best) << (i * 3);
    }

    for (size_t i = 0; i < 6; i++) {
        out[2 + i] = static_cast<uint8_t>(indices >> (i * 8));
    }
}

std::vector<uint8_t> compress(const Surface&               surface,
                              const assetconv::TextureKind kind) {
    const auto blocksX       = (surface.width + blockSize - 1) / blockSize;
    const auto blocksY       = (surface.height + blockSize - 1) / blockSize;
    const auto normal        = kind == assetconv::TextureKind::Normal;
    const auto bytesPerBlock = normal ? bc4BlockBytes * 2 : bc7BlockBytes;

    std::vector<uint8_t> out(static_cast<size_t>(blocksX) * blocksY *
                             bytesPerBlock);

    bc7enc_compress_block_params params{};
    bc7enc_compress_block_params_init(&params);
    // Colorspace error in RGB, not YCbCr: the engine samples these as data
    // (roughness, occlusion) as often as as colour.
    bc7enc_compress_block_params_init_linear_weights(&params);

    for (uint32_t by = 0; by < blocksY; by++) {
        for (uint32_t bx = 0; bx < blocksX; bx++) {
            const auto block =
                readBlock(surface, bx * blockSize, by * blockSize);
            auto* dst = &out[((static_cast<size_t>(by) * blocksX) + bx) *
                             bytesPerBlock];
            if (normal) {
                encodeBc4(block, 0, dst);
                encodeBc4(block, 1, dst + bc4BlockBytes);
            } else {
                bc7enc_compress_block(dst, block.data(), &params);
            }
        }
    }

    return out;
}

// Half-size resample. Colour is filtered in sRGB space; everything else is
// data and must be filtered linearly, or the mips shift in brightness.
Surface halve(const Surface& surface, const assetconv::TextureKind kind) {
    Surface next{ .width  = std::max(surface.width / 2, 1U),
                  .height = std::max(surface.height / 2, 1U),
                  .pixels = {} };
    next.pixels.resize(static_cast<size_t>(next.width) * next.height *
                       rgbaChannels);

    if (kind == assetconv::TextureKind::Color) {
        stbir_resize_uint8_srgb(
            surface.pixels.data(), static_cast<int>(surface.width),
            static_cast<int>(surface.height), 0, next.pixels.data(),
            static_cast<int>(next.width), static_cast<int>(next.height), 0,
            STBIR_RGBA);
    } else {
        stbir_resize_uint8_linear(
            surface.pixels.data(), static_cast<int>(surface.width),
            static_cast<int>(surface.height), 0, next.pixels.data(),
            static_cast<int>(next.width), static_cast<int>(next.height), 0,
            STBIR_RGBA);
    }
    return next;
}

ktx2::Format formatFor(const assetconv::TextureKind kind) {
    // BC7 UNORM, not SRGB: this matches the GL_RGBA8 the engine uploaded
    // before baking existed, so compression is the only change. Switching
    // albedo to sRGB is a separate, visible change.
    return kind == assetconv::TextureKind::Normal ? ktx2::formatBc5Unorm :
                                                    ktx2::formatBc7Unorm;
}
}  // namespace

namespace assetconv {

void initEncoder() {
    bc7enc_compress_block_init();
}

sponge::scene::ParsedImage loadImage(const std::string& path) {
    int   width   = 0;
    int   height  = 0;
    int   ignored = 0;
    auto* pixels  = stbi_load(path.c_str(), &width, &height, &ignored,
                              static_cast<int>(rgbaChannels));
    if (pixels == nullptr) {
        fmt::println(stderr, "assetconv: unable to load {}: {}", path,
                     stbi_failure_reason());
        return {};
    }

    sponge::scene::ParsedImage image{
        .name          = path,
        .width         = static_cast<uint32_t>(width),
        .height        = static_cast<uint32_t>(height),
        .bytesPerPixel = static_cast<uint32_t>(rgbaChannels),
        .pixels = { pixels, pixels + (static_cast<size_t>(width) * height *
                                      rgbaChannels) },
        .ktx2   = {},
    };
    stbi_image_free(pixels);
    return image;
}

std::vector<uint8_t> encode(const sponge::scene::ParsedImage& image,
                            const TextureKind                 kind) {
    if (image.width == 0 || image.height == 0 || image.pixels.empty()) {
        fmt::println(stderr, "assetconv: cannot encode empty image {}",
                     image.name);
        return {};
    }

    auto surface = toRgba(image);

    // Full chain down to 1x1. Compressed levels smaller than a block still
    // occupy one block, which is what glCompressedTexImage2D expects.
    std::vector<std::vector<uint8_t>> levels;
    while (true) {
        levels.emplace_back(compress(surface, kind));
        if (surface.width == 1 && surface.height == 1) {
            break;
        }
        surface = halve(surface, kind);
    }

    return ktx2::write(formatFor(kind), image.width, image.height, levels);
}

}  // namespace assetconv
