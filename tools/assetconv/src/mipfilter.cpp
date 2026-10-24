#include "mipfilter.hpp"

#include "parallel.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

constexpr size_t channels = 4;

// Floats per texel while filtering. Opaque or linear data needs R G B A.
// Colour with transparent texels also carries the plain R G B beside the
// alpha-weighted one, as R G B A Rw Gw Bw Aw: where the weighted alpha is 0
// only the plain colour says anything, and the engine draws albedo without
// alpha, so dropping it turns foliage cards black.
constexpr size_t singleFloats = channels;
constexpr size_t doubleFloats = channels * 2;

// Below this the weighted alpha is rounding noise and the plain colour is
// used instead.
constexpr float minAlpha = 1.0e-6F;

using Pixel = std::array<float, channels>;

// Mitchell-Netravali, B = C = 1/3. The weight is 0 from 2 on.
float mitchell(float x) {
    x = std::abs(x);
    if (x < 1.0F) {
        return (16.0F + x * x * (21.0F * x - 36.0F)) / 18.0F;
    }
    if (x < 2.0F) {
        return (32.0F + x * (-60.0F + x * (36.0F - 7.0F * x))) / 18.0F;
    }
    return 0.0F;
}

// The source texels that make one output texel, as a run from `first`.
struct Taps {
    size_t             first{ 0 };
    std::vector<float> weights;
};

std::vector<Taps> makeTaps(const uint32_t srcSize, const uint32_t dstSize) {
    std::vector<Taps> taps(dstSize);

    // Only a one-pixel axis keeps its size. A filter would blur it.
    if (srcSize == dstSize) {
        for (uint32_t i = 0; i < dstSize; i++) {
            taps[i] = { .first = i, .weights = { 1.0F } };
        }
        return taps;
    }

    const auto scale =
        static_cast<float>(srcSize) / static_cast<float>(dstSize);
    const auto radius = 2.0F * scale;
    const auto last   = static_cast<int64_t>(srcSize) - 1;

    for (uint32_t i = 0; i < dstSize; i++) {
        const auto centre = (static_cast<float>(i) + 0.5F) * scale;
        const auto lo     = static_cast<int64_t>(std::floor(centre - radius));
        const auto hi     = static_cast<int64_t>(std::ceil(centre + radius));
        const auto first  = std::clamp<int64_t>(lo, 0, last);

        auto& tap = taps[i];
        tap.first = static_cast<size_t>(first);
        tap.weights.assign(
            static_cast<size_t>(std::clamp<int64_t>(hi, 0, last) - first + 1),
            0.0F);

        // Taps past the border fold into the edge texel.
        float sum = 0.0F;
        for (int64_t k = lo; k <= hi; k++) {
            const auto weight =
                mitchell((static_cast<float>(k) + 0.5F - centre) / scale);
            tap.weights[static_cast<size_t>(std::clamp<int64_t>(k, 0, last) -
                                            first)] += weight;
            sum += weight;
        }
        for (auto& weight : tap.weights) {
            weight /= sum;
        }
    }
    return taps;
}

const std::array<float, 256>& srgbToLinear() {
    static const auto table = [] {
        std::array<float, 256> out{};
        for (size_t i = 0; i < out.size(); i++) {
            const auto v = static_cast<float>(i) / 255.0F;
            out[i] = v <= 0.04045F ? v / 12.92F :
                                     std::pow((v + 0.055F) / 1.055F, 2.4F);
        }
        return out;
    }();
    return table;
}

uint8_t toByte(const float v) {
    return static_cast<uint8_t>((std::clamp(v, 0.0F, 1.0F) * 255.0F) + 0.5F);
}

// Linear light to an sRGB byte, indexed by linear * srgbSteps. A table,
// because pow per texel costs more than the whole filter. The steps are fine
// enough that a lookup is within one level of the exact curve, even in the
// darks where the curve is steep.
constexpr size_t srgbSteps = 16384;

const std::array<uint8_t, srgbSteps + 1>& linearToSrgbTable() {
    static const auto table = [] {
        std::array<uint8_t, srgbSteps + 1> out{};
        for (size_t i = 0; i < out.size(); i++) {
            const auto l =
                static_cast<float>(i) / static_cast<float>(srgbSteps);
            out[i] = toByte(l <= 0.0031308F ?
                                l * 12.92F :
                                (1.055F * std::pow(l, 1.0F / 2.4F)) - 0.055F);
        }
        return out;
    }();
    return table;
}

// Source row to floats. Colour is decoded and multiplied by alpha, and kept
// plain as well when there are two sets of channels.
template <size_t Floats>
void decodeRow(const uint8_t* src, const size_t width,
               const assetconv::MipSpace space, float* out) {
    constexpr float byteScale = 1.0F / 255.0F;

    if (space == assetconv::MipSpace::Srgb) {
        const auto& lut = srgbToLinear();
        for (size_t x = 0; x < width; x++) {
            const auto* in    = src + (x * channels);
            auto*       dst   = out + (x * Floats);
            const auto  alpha = static_cast<float>(in[3]) * byteScale;
            const Pixel texel{ lut[in[0]], lut[in[1]], lut[in[2]], 1.0F };
            if constexpr (Floats == doubleFloats) {
                dst[0] = texel[0];
                dst[1] = texel[1];
                dst[2] = texel[2];
                dst[3] = alpha;
            }
            for (size_t c = 0; c < channels; c++) {
                dst[Floats - channels + c] = texel[c] * alpha;
            }
        }
        return;
    }

    // One flat loop, so the compiler widens the bytes in bulk.
    for (size_t i = 0; i < width * channels; i++) {
        out[i] = static_cast<float>(src[i]) * byteScale;
    }
}

// Output row back to bytes. Colour is divided by alpha and encoded. Where
// alpha is 0 the plain colour is used, if there is one.
template <size_t Floats>
void encodeRow(const float* src, const size_t width,
               const assetconv::MipSpace space, uint8_t* out) {
    if (space == assetconv::MipSpace::Srgb) {
        const auto& table = linearToSrgbTable();
        for (size_t x = 0; x < width; x++) {
            const auto* in    = src + (x * Floats);
            auto*       dst   = out + (x * channels);
            const auto  alpha = std::clamp(in[3], 0.0F, 1.0F);

            // Undo the alpha weight, clamp and scale to table indices.
            for (size_t c = 0; c < 3; c++) {
                float unit = 0.0F;
                if (alpha >= minAlpha) {
                    unit = in[Floats - channels + c] * (1.0F / alpha);
                } else if constexpr (Floats == doubleFloats) {
                    unit = in[c];
                }
                dst[c] =
                    table[static_cast<size_t>((std::clamp(unit, 0.0F, 1.0F) *
                                               static_cast<float>(srgbSteps)) +
                                              0.5F)];
            }
            dst[3] = toByte(alpha);
        }
        return;
    }

    for (size_t i = 0; i < width * channels; i++) {
        out[i] = toByte(src[i]);
    }
}

// One source row, filtered across its width.
template <size_t Floats>
void filterRow(const std::vector<float>& decoded, const std::vector<Taps>& taps,
               float* out) {
    for (size_t x = 0; x < taps.size(); x++) {
        const auto& tap = taps[x];
        const auto* in  = &decoded[tap.first * Floats];

        std::array<float, Floats> acc{};
        for (size_t k = 0; k < tap.weights.size(); k++) {
            for (size_t i = 0; i < Floats; i++) {
                acc[i] += in[(k * Floats) + i] * tap.weights[k];
            }
        }
        std::ranges::copy(acc, out + (x * Floats));
    }
}

// True if any texel is not fully opaque.
bool hasTransparency(const std::span<const uint8_t> pixels) {
    for (size_t i = channels - 1; i < pixels.size(); i += channels) {
        if (pixels[i] != 255) {
            return true;
        }
    }
    return false;
}

// Output rows [yBegin, yEnd) of a halved image. Source rows are filtered
// across once, in order, and kept in a ring just big enough for the widest
// vertical window. Whole-image intermediates would cost hundreds of MB on a 4K
// texture.
template <size_t Floats>
void halveBand(const std::span<const uint8_t> pixels, const uint32_t width,
               const assetconv::MipSpace space,
               const std::vector<Taps>& columns, const std::vector<Taps>& rows,
               const size_t dstWidth, const size_t yBegin, const size_t yEnd,
               std::vector<uint8_t>& out) {
    size_t window = 0;
    for (const auto& row : rows) {
        window = std::max(window, row.weights.size());
    }
    const auto stride = dstWidth * Floats;

    std::vector<float> ring(window * stride);
    std::vector<float> line(stride);
    std::vector<float> decoded(static_cast<size_t>(width) * Floats);

    // Where each row of the current output row's vertical window sits in the
    // ring. Set up once per row so the inner loop does no index maths.
    std::vector<const float*> source(window);

    size_t filtered = rows[yBegin].first;
    for (size_t y = yBegin; y < yEnd; y++) {
        const auto& tap = rows[y];

        for (; filtered < tap.first + tap.weights.size(); filtered++) {
            decodeRow<Floats>(&pixels[filtered * width * channels], width,
                              space, decoded.data());
            filterRow<Floats>(decoded, columns,
                              &ring[(filtered % window) * stride]);
        }

        const auto count = tap.weights.size();
        for (size_t k = 0; k < count; k++) {
            source[k] = &ring[((tap.first + k) % window) * stride];
        }
        for (size_t i = 0; i < stride; i++) {
            float acc = 0.0F;
            for (size_t k = 0; k < count; k++) {
                acc += tap.weights[k] * source[k][i];
            }
            line[i] = acc;
        }

        encodeRow<Floats>(line.data(), dstWidth, space,
                          &out[y * dstWidth * channels]);
    }
}

template <size_t Floats>
std::vector<uint8_t> halve(const std::span<const uint8_t> pixels,
                           const uint32_t width, const uint32_t height,
                           const assetconv::MipSpace space) {
    const auto dstWidth  = std::max(width / 2, 1U);
    const auto dstHeight = std::max(height / 2, 1U);
    const auto columns   = makeTaps(width, dstWidth);
    const auto rows      = makeTaps(height, dstHeight);

    // Rows below this many per band are not worth a thread.
    constexpr size_t minBandRows = 64;
    const auto       bands       = std::max<size_t>(dstHeight / minBandRows, 1);

    std::vector<uint8_t> out(static_cast<size_t>(dstWidth) * dstHeight *
                             channels);

    // Output rows are independent, so each band of them is filtered on its
    // own thread. A band starts by filtering the few source rows that its
    // first window needs, which its neighbour also filters; the bytes are the
    // same as one pass over the whole image.
    assetconv::parallelFor(bands, [&](const size_t band) {
        halveBand<Floats>(pixels, width, space, columns, rows, dstWidth,
                          dstHeight * band / bands,
                          dstHeight * (band + 1) / bands, out);
    });
    return out;
}

}  // namespace

namespace assetconv {

std::vector<uint8_t> halveMip(const std::span<const uint8_t> pixels,
                              const uint32_t width, const uint32_t height,
                              const MipSpace space) {
    // Only colour with transparent texels needs the second set of channels.
    if (space == MipSpace::Srgb && hasTransparency(pixels)) {
        return halve<doubleFloats>(pixels, width, height, space);
    }
    return halve<singleFloats>(pixels, width, height, space);
}

double alphaCoverage(const std::span<const uint8_t> pixels,
                     const uint8_t                  threshold) {
    size_t passing = 0;
    for (size_t i = 3; i < pixels.size(); i += channels) {
        passing += pixels[i] >= threshold ? 1 : 0;
    }
    const auto texels = pixels.size() / channels;
    return texels == 0 ?
               0.0 :
               static_cast<double>(passing) / static_cast<double>(texels);
}

void scaleAlphaCoverage(const std::span<uint8_t> pixels,
                        const uint8_t threshold, const double target) {
    const auto texels = pixels.size() / channels;
    if (texels == 0 || target <= 0.0 || target >= 1.0) {
        return;
    }

    std::array<size_t, 256> histogram{};
    for (size_t i = 3; i < pixels.size(); i += channels) {
        histogram[pixels[i]]++;
    }

    // The largest alpha that, taken as the new cutoff, still leaves enough
    // texels passing. Scaling it up to the real threshold does the same to
    // every alpha.
    const auto need =
        static_cast<size_t>(std::ceil(target * static_cast<double>(texels)));
    size_t passing = 0;
    size_t t       = 255;
    for (; t > 1; t--) {
        passing += histogram[t];
        if (passing >= need) {
            break;
        }
    }
    const auto scale = static_cast<double>(threshold) / static_cast<double>(t);
    for (size_t i = 3; i < pixels.size(); i += channels) {
        pixels[i] = static_cast<uint8_t>(std::min(
            255.0, std::round(static_cast<double>(pixels[i]) * scale)));
    }
}

}  // namespace assetconv
