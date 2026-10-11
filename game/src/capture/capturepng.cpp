#include "capture/capturepng.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <vector>

#include <spng.h>

namespace game::capture {
using sponge::platform::opengl::renderer::Image;

namespace {
struct SpngDeleter {
    void operator()(spng_ctx* context) const {
        spng_ctx_free(context);
    }
};

// Written as comparisons so a NaN lands on a value in range: the build uses
// fast math.
uint8_t toByte(const float value) {
    const float clamped = value > 1.F ? 1.F : (value > 0.F ? value : 0.F);
    return static_cast<uint8_t>((clamped * 255.F) + 0.5F);
}
}  // namespace

// The image holds rows bottom to top, a PNG top to bottom.
bool writePng(const Image& image, const char* path) {
    constexpr size_t     channels = 3;
    const size_t         rowSize  = static_cast<size_t>(image.width) * channels;
    std::vector<uint8_t> pixels(image.rgb.size());
    for (size_t y = 0; y < image.height; y++) {
        const auto* src = &image.rgb[(image.height - 1 - y) * rowSize];
        auto*       dst = &pixels[y * rowSize];
        for (size_t i = 0; i < rowSize; i++) {
            dst[i] = toByte(src[i]);
        }
    }

    const std::unique_ptr<spng_ctx, SpngDeleter> context{ spng_ctx_new(
        SPNG_CTX_ENCODER) };
    if (context == nullptr) {
        return false;
    }
    spng_ihdr ihdr{};
    ihdr.width      = image.width;
    ihdr.height     = image.height;
    ihdr.bit_depth  = 8;
    ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR;
    if (spng_set_option(context.get(), SPNG_ENCODE_TO_BUFFER, 1) != 0 ||
        spng_set_ihdr(context.get(), &ihdr) != 0 ||
        spng_encode_image(context.get(), pixels.data(), pixels.size(),
                          SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE) != 0) {
        return false;
    }

    int                                               error = 0;
    size_t                                            size  = 0;
    const std::unique_ptr<void, decltype(&std::free)> png{
        spng_get_png_buffer(context.get(), &size, &error), &std::free
    };
    if (png == nullptr) {
        return false;
    }
    std::ofstream out(path, std::ios::binary);
    out.write(static_cast<const char*>(png.get()),
              static_cast<std::streamsize>(size));
    return out.good();
}
}  // namespace game::capture
