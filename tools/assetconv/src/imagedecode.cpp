#include "imagedecode.hpp"

#include <fmt/base.h>
#include <spng.h>
#include <turbojpeg.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {
using sponge::scene::ParsedImage;

constexpr uint32_t rgbaChannels = 4;

constexpr std::array<uint8_t, 4> pngMagic{ 0x89, 'P', 'N', 'G' };
constexpr std::array<uint8_t, 3> jpegMagic{ 0xFF, 0xD8, 0xFF };

struct SpngDeleter {
    void operator()(spng_ctx* context) const {
        spng_ctx_free(context);
    }
};

struct TurboDeleter {
    void operator()(void* handle) const {
        tj3Destroy(handle);
    }
};

bool startsWith(const std::span<const uint8_t> bytes, const auto& magic) {
    return bytes.size() >= magic.size() &&
           std::equal(magic.begin(), magic.end(), bytes.begin());
}

std::optional<ParsedImage> decodePng(const std::span<const uint8_t> bytes,
                                     const std::string&             name) {
    const std::unique_ptr<spng_ctx, SpngDeleter> context{ spng_ctx_new(0) };
    if (context == nullptr) {
        fmt::println(stderr, "assetconv: unable to decode {}: out of memory",
                     name);
        return std::nullopt;
    }

    spng_ihdr ihdr{};
    size_t    size = 0;
    auto error = spng_set_png_buffer(context.get(), bytes.data(), bytes.size());
    if (error == 0) {
        error = spng_get_ihdr(context.get(), &ihdr);
    }
    if (error == 0) {
        error = spng_decoded_image_size(context.get(), SPNG_FMT_RGBA8, &size);
    }
    std::vector<uint8_t> pixels(size);
    if (error == 0) {
        // tRNS becomes alpha. Gamma is left alone.
        error = spng_decode_image(context.get(), pixels.data(), size,
                                  SPNG_FMT_RGBA8, SPNG_DECODE_TRNS);
    }
    if (error != 0) {
        fmt::println(stderr, "assetconv: unable to decode {}: {}", name,
                     spng_strerror(error));
        return std::nullopt;
    }

    return ParsedImage{
        .name          = name,
        .width         = ihdr.width,
        .height        = ihdr.height,
        .bytesPerPixel = rgbaChannels,
        .pixels        = std::move(pixels),
        .ktx2          = {},
    };
}

std::optional<ParsedImage> decodeJpeg(const std::span<const uint8_t> bytes,
                                      const std::string&             name) {
    const std::unique_ptr<void, TurboDeleter> handle{ tj3Init(
        TJINIT_DECOMPRESS) };
    if (handle == nullptr ||
        tj3DecompressHeader(handle.get(), bytes.data(), bytes.size()) != 0) {
        fmt::println(stderr, "assetconv: unable to decode {}: {}", name,
                     tj3GetErrorStr(handle.get()));
        return std::nullopt;
    }

    const auto width =
        static_cast<uint32_t>(tj3Get(handle.get(), TJPARAM_JPEGWIDTH));
    const auto height =
        static_cast<uint32_t>(tj3Get(handle.get(), TJPARAM_JPEGHEIGHT));
    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height *
                                rgbaChannels);
    if (tj3Decompress8(handle.get(), bytes.data(), bytes.size(), pixels.data(),
                       0, TJPF_RGBA) != 0) {
        fmt::println(stderr, "assetconv: unable to decode {}: {}", name,
                     tj3GetErrorStr(handle.get()));
        return std::nullopt;
    }

    return ParsedImage{
        .name          = name,
        .width         = width,
        .height        = height,
        .bytesPerPixel = rgbaChannels,
        .pixels        = std::move(pixels),
        .ktx2          = {},
    };
}
}  // namespace

namespace assetconv {

std::optional<ParsedImage> decodeImage(const std::span<const uint8_t> bytes,
                                       const std::string&             name) {
    if (startsWith(bytes, pngMagic)) {
        return decodePng(bytes, name);
    }
    if (startsWith(bytes, jpegMagic)) {
        return decodeJpeg(bytes, name);
    }
    fmt::println(stderr, "assetconv: unable to decode {}: not a PNG or JPEG",
                 name);
    return std::nullopt;
}

}  // namespace assetconv
