#pragma once

#include "platform/opengl/debug/gputimer.hpp"
#include "platform/opengl/renderer/readback.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace game::layer {

// Debug capture: counts non-finite, huge and isolated pixels per pipeline
// stage, every frame, on 32-bit float readbacks. Stages read before the final
// 8-bit write, so a NaN or Inf shows up as itself instead of as a white or
// black pixel. Render thread only.
class FrameCapture {
public:
    FrameCapture(uint32_t frames, bool stats) : frames(frames), stats(stats) {}

    // True once `frames` frames have been recorded.
    bool done() const {
        return frame >= frames;
    }

    // Call once per rendered frame, before the stage reads.
    void beginFrame() {
        const auto now = std::chrono::steady_clock::now();
        if (frame > 0) {
            frameMs.push_back(
                std::chrono::duration<double, std::milli>(now - last).count());
        }
        last = now;
        frame++;
    }

    // `hdr` stages hold unbounded radiance, the rest hold 0..1 display values.
    void record(const std::string&                               stage,
                const sponge::platform::opengl::renderer::Image& image,
                bool                                             hdr);

    bool isLastFrame() const {
        return frame == frames;
    }

    bool wantsStats() const {
        return stats;
    }

    // Writes one line per stage to capture_stats.txt, and the frame-time
    // summary with one line per timed pass to capture_timing.txt.
    void report(
        const sponge::platform::opengl::debug::GpuTimer::Summary& gpu) const;

private:
    struct Stage {
        std::string name;
        uint64_t    pixels     = 0;
        uint64_t    nan        = 0;
        uint64_t    inf        = 0;
        uint64_t    big        = 0;
        uint64_t    bright     = 0;
        uint64_t    dark       = 0;
        uint32_t    hitFrames  = 0;
        uint32_t    firstFrame = 0;
        uint32_t    firstX     = 0;
        uint32_t    firstY     = 0;
        float       maxFinite  = 0.F;
        float       firstLuma  = 0.F;
    };

    uint32_t frames = 0;
    bool     stats  = false;
    uint32_t frame  = 0;

    // Wall time between consecutive beginFrame() calls.
    std::chrono::steady_clock::time_point last;
    std::vector<double>                   frameMs;

    std::vector<Stage> stages;

    // Frame, x, y of each speck in the scene stage, for capture_specks.txt.
    std::vector<std::array<uint32_t, 3>> specks;
};

}  // namespace game::layer
