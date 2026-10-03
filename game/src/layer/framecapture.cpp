#include "layer/framecapture.hpp"

#include "platform/opengl/renderer/readback.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <limits>

namespace game::layer {
using sponge::platform::opengl::renderer::Image;
using sponge::platform::opengl::renderer::writePfm;

namespace {
// Radiance this high is no real highlight. Half float tops out at 65504.
constexpr float bigRadiance = 1000.F;
// A pixel is a speck when it is this many times brighter than its brightest
// neighbour, or this many times darker than its darkest one. The floors keep
// flat dark and flat bright areas out.
constexpr float  speckRatio      = 3.F;
constexpr float  darkRatio       = 0.1F;
constexpr size_t maxSpecksLogged = 5000;

// The build uses fast math, so the compiler may assume there is no NaN or Inf
// and fold std::isnan and std::isinf to false. Test the bits instead, and read
// them through a pointer: a float passed by value is assumed finite too, which
// folds a bit test on it away.
constexpr uint32_t exponentMask = 0x7F800000U;
constexpr uint32_t mantissaMask = 0x007FFFFFU;

uint32_t bitsOf(const float* v) {
    uint32_t bits = 0;
    std::memcpy(&bits, v, sizeof(bits));
    return bits;
}

bool isNaN(const float* v) {
    const auto bits = bitsOf(v);
    return (bits & exponentMask) == exponentMask && (bits & mantissaMask) != 0;
}

bool isInf(const float* v) {
    const auto bits = bitsOf(v);
    return (bits & exponentMask) == exponentMask && (bits & mantissaMask) == 0;
}

float luma(const float* p) {
    return 0.2126F * p[0] + 0.7152F * p[1] + 0.0722F * p[2];
}
}  // namespace

void FrameCapture::record(const std::string& stage, const Image& image,
                          const bool hdr) {
    auto it = std::ranges::find(stages, stage, &Stage::name);
    if (it == stages.end()) {
        it = stages.insert(stages.end(), Stage{ .name = stage });
    }
    Stage& s = *it;

    bool hit = false;
    if (stats && image.width > 2 && image.height > 2) {
        const auto w = static_cast<size_t>(image.width);
        const auto h = static_cast<size_t>(image.height);

        // Non-finite pixels read as black, which is what the tone map shows.
        std::vector<float> plane(w * h, 0.F);
        const auto         noteFirst = [&](const size_t x, const size_t y) {
            if (!hit && s.hitFrames == 0) {
                s.firstFrame = frame;
                s.firstX     = static_cast<uint32_t>(x);
                s.firstY     = static_cast<uint32_t>(y);
                s.firstLuma  = luma(&image.rgb[(y * w + x) * 3]);
            }
        };

        for (size_t y = 0; y < h; y++) {
            for (size_t x = 0; x < w; x++) {
                const float* p = &image.rgb[(y * w + x) * 3];
                if (isNaN(p) || isNaN(p + 1) || isNaN(p + 2)) {
                    s.nan++;
                    noteFirst(x, y);
                    hit = true;
                } else if (isInf(p) || isInf(p + 1) || isInf(p + 2)) {
                    s.inf++;
                    noteFirst(x, y);
                    hit = true;
                } else {
                    const float peak = std::max({ p[0], p[1], p[2] });
                    s.maxFinite      = std::max(s.maxFinite, peak);
                    plane[y * w + x] = luma(p);
                    if (peak > bigRadiance) {
                        s.big++;
                        noteFirst(x, y);
                        hit = true;
                    }
                }
            }
        }

        const float brightFloor = hdr ? 10.F : 0.1F;
        const float darkFloor   = hdr ? 1.F : 0.2F;
        for (size_t y = 1; y + 1 < h; y++) {
            for (size_t x = 1; x + 1 < w; x++) {
                float maxN = 0.F;
                float minN = std::numeric_limits<float>::max();
                for (size_t dy = 0; dy < 3; dy++) {
                    for (size_t dx = 0; dx < 3; dx++) {
                        if (dx == 1 && dy == 1) {
                            continue;
                        }
                        const float l = plane[(y + dy - 1) * w + x + dx - 1];
                        maxN          = std::max(maxN, l);
                        minN          = std::min(minN, l);
                    }
                }
                const float l = plane[y * w + x];
                if (l > speckRatio * maxN + brightFloor) {
                    s.bright++;
                    if (stage == "scene" && specks.size() < maxSpecksLogged) {
                        specks.push_back({ frame, static_cast<uint32_t>(x),
                                           static_cast<uint32_t>(y) });
                    }
                    noteFirst(x, y);
                    hit = true;
                } else if (minN > darkFloor && l < darkRatio * minN) {
                    s.dark++;
                    noteFirst(x, y);
                    hit = true;
                }
            }
        }
        s.pixels += w * h;
    }

    if (hit) {
        if (s.hitFrames == 0) {
            writePfm(image,
                     std::format("capture_{}_first_hit.pfm", stage).c_str());
        }
        s.hitFrames++;
    }
    if (isLastFrame()) {
        writePfm(image, std::format("capture_{}_last.pfm", stage).c_str());
    }
}

void FrameCapture::report(
    const sponge::platform::opengl::debug::GpuTimer::Summary& gpu) const {
    // A file, not the log: release builds compile SPONGE_INFO out.
    std::ofstream out("capture_stats.txt");
    out << std::format("frames={}\n", std::min(frame, frames));
    for (const auto& s : stages) {
        out << std::format(
            "{:<10} hitFrames={} nan={} inf={} big={} bright={} dark={} "
            "maxFinite={} first=frame {} at ({}, {}) luma {}\n",
            s.name, s.hitFrames, s.nan, s.inf, s.big, s.bright, s.dark,
            s.maxFinite, s.firstFrame, s.firstX, s.firstY, s.firstLuma);
    }

    // Without stats only the last frame stalls on a readback, so drop it. The
    // first frames hold shader warm-up and the probe capture.
    constexpr size_t warmupFrames = 60;
    if (!stats && frameMs.size() > warmupFrames + 1) {
        std::vector<double> ms(frameMs.begin() + warmupFrames,
                               frameMs.end() - 1);
        std::ranges::sort(ms);
        double sum = 0.;
        for (const double v : ms) {
            sum += v;
        }
        std::ofstream timing("capture_timing.txt");
        timing << std::format("frames={} mean={:.2f}ms p50={:.2f}ms "
                              "p95={:.2f}ms fps={:.1f}\n",
                              ms.size(), sum / static_cast<double>(ms.size()),
                              ms[ms.size() / 2], ms[ms.size() * 95 / 100],
                              1000. * static_cast<double>(ms.size()) / sum);
        const auto& f = gpu.frame;
        if (f.frames > 0) {
            timing << std::format(
                "gpu frame      span p50={:.3f}ms mean={:.3f}ms  period "
                "p50={:.3f}ms mean={:.3f}ms\n",
                f.spanMedian, f.spanMean, f.periodMedian, f.periodMean);
        }
        for (const auto& s : gpu.passes) {
            timing << std::format(
                "gpu {:<10} calls={} p50={:.3f}ms perFrame={:.3f}ms\n", s.name,
                s.calls, s.medianMs, s.perFrameMs);
        }
    }

    std::ofstream specksOut("capture_specks.txt");
    for (const auto& [f, x, y] : specks) {
        specksOut << f << ' ' << x << ' ' << y << '\n';
    }
}

}  // namespace game::layer
