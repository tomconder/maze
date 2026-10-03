#pragma once

#include "platform/opengl/renderer/gl.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sponge::platform::opengl::debug {

// GPU time per named pass, from GL_TIME_ELAPSED queries, and the GPU time of
// the whole frame, from GL_TIMESTAMP queries. Passes must not nest: GL allows
// one active query per target. Results are read a few frames after they are
// issued, so the timer never stalls the pipeline. Render thread only.
class GpuTimer {
public:
    struct Stat {
        std::string_view name;
        uint32_t         calls = 0;
        // Median GPU time of one call, and the total divided by the frames
        // counted. The two differ for a pass that runs on some frames only.
        double medianMs   = 0.;
        double perFrameMs = 0.;
    };

    // GPU clock times between nextFrame() and endFrame() (span), and between
    // two nextFrame() calls (period). The period minus the span is the time
    // the GPU spent outside the frame: idle, waiting on the swap, or in work
    // issued after endFrame().
    struct FrameStat {
        uint32_t frames       = 0;
        double   spanMedian   = 0.;
        double   spanMean     = 0.;
        double   periodMedian = 0.;
        double   periodMean   = 0.;
    };

    struct Summary {
        // In order of first use.
        std::vector<Stat> passes;
        FrameStat         frame;
    };

    GpuTimer();
    ~GpuTimer();

    GpuTimer(const GpuTimer&)            = delete;
    GpuTimer& operator=(const GpuTimer&) = delete;

    // Once per frame, before any begin(). Stamps the start of the frame and
    // reads the results of the frame that used this slot `depth` frames ago.
    void nextFrame();

    // Stamps the end of the frame, after the last pass.
    void endFrame();

    // `name` must outlive the timer: pass a string literal.
    void begin(std::string_view name);
    void end();

    // Over the frames from `warmupFrames` on.
    Summary summary(uint32_t warmupFrames) const;

private:
    // The GPU runs at most a few frames behind the CPU.
    static constexpr size_t depth       = 4;
    static constexpr size_t maxPerFrame = 24;

    struct Sample {
        uint32_t         frame;
        std::string_view name;
        double           ms;
    };

    struct FrameSample {
        uint32_t frame;
        double   spanMs;
        double   periodMs;
    };

    std::array<GLuint, depth * maxPerFrame>           queries{};
    std::array<std::string_view, depth * maxPerFrame> names{};
    std::array<size_t, depth>                         counts{};
    std::vector<Sample>                               samples;
    uint32_t                                          frame = 0;
    bool                                              open  = false;

    // Frame number held by each ring slot, for the sample's frame field.
    std::array<uint32_t, depth> slotFrame{};

    // Start and end timestamp of each slot's frame. `stamped` is set once the
    // end is issued, so a slot that never ended is not read.
    std::array<GLuint, depth * 2> stamps{};
    std::array<bool, depth>       stamped{};
    GLuint64                      previousStartNs = 0;
    std::vector<FrameSample>      frameSamples;
};

}  // namespace sponge::platform::opengl::debug
