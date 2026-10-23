#include "hdrbake.hpp"

#include "ktx2.hpp"
#include "readbytes.hpp"

#include <fmt/base.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr size_t   rgbeBytes   = 4;
constexpr size_t   rgbaFloats  = 4;
constexpr float    halfMax     = 65504.F;
constexpr uint32_t minRleWidth = 8;
constexpr uint32_t maxRleWidth = 32767;

// Float to IEEE half, rounding toward zero. Radiance values are positive and
// the image is a light source, so the lost low bit does not matter.
uint16_t toHalf(float value) {
    value               = std::clamp(value, 0.F, halfMax);
    const auto bits     = std::bit_cast<uint32_t>(value);
    const auto exponent = static_cast<int>((bits >> 23) & 0xFF) - 127 + 15;
    if (exponent <= 0) {
        return 0;  // below the smallest half: a black texel is close enough
    }
    return static_cast<uint16_t>((exponent << 10) | ((bits >> 13) & 0x3FF));
}

// Reads one header line, without its newline. False at the end of the file.
bool readLine(const std::span<const uint8_t> bytes, size_t& cursor,
              std::string& line) {
    line.clear();
    if (cursor >= bytes.size()) {
        return false;
    }
    while (cursor < bytes.size() && bytes[cursor] != '\n') {
        line.push_back(static_cast<char>(bytes[cursor++]));
    }
    cursor++;
    return true;
}

// One scanline in the new run-length format: the four bytes of each pixel
// are stored as four separate runs.
bool readRleScanline(const std::span<const uint8_t> bytes, size_t& cursor,
                     const uint32_t width, uint8_t* scanline) {
    for (size_t channel = 0; channel < rgbeBytes; channel++) {
        uint32_t x = 0;
        while (x < width) {
            if (cursor >= bytes.size()) {
                return false;
            }
            auto count = static_cast<uint32_t>(bytes[cursor++]);
            if (count > 128) {
                count -= 128;
                if (cursor >= bytes.size() || x + count > width) {
                    return false;
                }
                const auto value = bytes[cursor++];
                for (uint32_t i = 0; i < count; i++) {
                    scanline[(x++ * rgbeBytes) + channel] = value;
                }
            } else {
                if (count == 0 || cursor + count > bytes.size() ||
                    x + count > width) {
                    return false;
                }
                for (uint32_t i = 0; i < count; i++) {
                    scanline[(x++ * rgbeBytes) + channel] = bytes[cursor++];
                }
            }
        }
    }
    return true;
}
}  // namespace

namespace assetconv {

std::vector<uint8_t> bakeEnvironment(const std::string& source) {
    const auto bytes = sponge::scene::readBytes(source);
    if (bytes.empty()) {
        fmt::println(stderr, "assetconv: unable to read {}", source);
        return {};
    }

    size_t      cursor = 0;
    std::string line;
    if (!readLine(bytes, cursor, line) || !line.starts_with("#?")) {
        fmt::println(stderr, "assetconv: {} is not a Radiance image", source);
        return {};
    }
    // The header ends at a blank line; the resolution follows.
    bool rgbe = false;
    while (readLine(bytes, cursor, line) && !line.empty()) {
        rgbe = rgbe || line == "FORMAT=32-bit_rle_rgbe";
    }
    uint32_t width  = 0;
    uint32_t height = 0;
    readLine(bytes, cursor, line);
    // Only the usual top-down, left-to-right order.
    std::istringstream resolution(line);
    std::string        yAxis;
    std::string        xAxis;
    resolution >> yAxis >> height >> xAxis >> width;
    if (!rgbe || resolution.fail() || yAxis != "-Y" || xAxis != "+X" ||
        width == 0 || height == 0) {
        fmt::println(stderr,
                     "assetconv: {} needs 32-bit_rle_rgbe and a '-Y h +X w' "
                     "resolution line",
                     source);
        return {};
    }

    std::vector<uint8_t> rgbeLine(static_cast<size_t>(width) * rgbeBytes);
    std::vector<uint8_t> out(static_cast<size_t>(width) * height * rgbaFloats *
                             sizeof(uint16_t));
    auto*                texel = reinterpret_cast<uint16_t*>(out.data());
    const auto           one   = toHalf(1.F);

    for (uint32_t y = 0; y < height; y++) {
        const auto rest = bytes.size() - std::min(cursor, bytes.size());
        // Scanlines start with 2, 2 and the width, in a width this format
        // can run-length encode. Anything else is flat pixels.
        const bool rle = rest >= rgbeBytes && width >= minRleWidth &&
                         width <= maxRleWidth && bytes[cursor] == 2 &&
                         bytes[cursor + 1] == 2 &&
                         ((bytes[cursor + 2] << 8) | bytes[cursor + 3]) ==
                             static_cast<int>(width);
        if (rle) {
            cursor += rgbeBytes;
            if (!readRleScanline(bytes, cursor, width, rgbeLine.data())) {
                fmt::println(stderr, "assetconv: {} is truncated at row {}",
                             source, y);
                return {};
            }
        } else {
            if (rest < rgbeLine.size()) {
                fmt::println(stderr, "assetconv: {} is truncated at row {}",
                             source, y);
                return {};
            }
            std::copy_n(bytes.begin() + static_cast<ptrdiff_t>(cursor),
                        rgbeLine.size(), rgbeLine.begin());
            cursor += rgbeLine.size();
        }

        for (uint32_t x = 0; x < width; x++) {
            const auto* p = &rgbeLine[static_cast<size_t>(x) * rgbeBytes];
            // value = mantissa / 256 * 2^(e - 128), and e == 0 is black.
            const float scale =
                p[3] == 0 ?
                    0.F :
                    std::ldexp(1.F / 256.F, static_cast<int>(p[3]) - 128);
            *texel++ = toHalf(static_cast<float>(p[0]) * scale);
            *texel++ = toHalf(static_cast<float>(p[1]) * scale);
            *texel++ = toHalf(static_cast<float>(p[2]) * scale);
            *texel++ = one;
        }
    }

    const std::array<std::vector<uint8_t>, 1> levels{ std::move(out) };
    const auto                                file = sponge::scene::ktx2::write(
        sponge::scene::ktx2::formatR16G16B16A16Sfloat, width, height, levels);
    fmt::println("{} ({}x{} RGBE) -> half float", source, width, height);
    return file;
}

}  // namespace assetconv
