#include "platform/opengl/debug/gputimer.hpp"

#include "platform/opengl/renderer/gl.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sponge::platform::opengl::debug {

namespace {
double median(std::vector<double> v) {
    if (v.empty()) {
        return 0.;
    }
    std::ranges::sort(v);
    return v[v.size() / 2];
}

double mean(const std::vector<double>& v) {
    double total = 0.;
    for (const double x : v) {
        total += x;
    }
    return v.empty() ? 0. : total / static_cast<double>(v.size());
}
}  // namespace

GpuTimer::GpuTimer() {
    glCreateQueries(GL_TIME_ELAPSED, static_cast<GLsizei>(queries.size()),
                    queries.data());
    glCreateQueries(GL_TIMESTAMP, static_cast<GLsizei>(stamps.size()),
                    stamps.data());
}

GpuTimer::~GpuTimer() {
    glDeleteQueries(static_cast<GLsizei>(queries.size()), queries.data());
    glDeleteQueries(static_cast<GLsizei>(stamps.size()), stamps.data());
}

void GpuTimer::nextFrame() {
    assert(!open);
    frame++;
    const size_t slot = frame % depth;

    // Results of the frame that last used this slot.
    for (size_t i = 0; i < counts[slot]; i++) {
        GLuint64 ns = 0;
        glGetQueryObjectui64v(queries[slot * maxPerFrame + i], GL_QUERY_RESULT,
                              &ns);
        samples.push_back({ slotFrame[slot], names[slot * maxPerFrame + i],
                            static_cast<double>(ns) / 1e6 });
    }
    if (stamped[slot]) {
        GLuint64 startNs = 0;
        GLuint64 endNs   = 0;
        glGetQueryObjectui64v(stamps[slot * 2], GL_QUERY_RESULT, &startNs);
        glGetQueryObjectui64v(stamps[slot * 2 + 1], GL_QUERY_RESULT, &endNs);
        // The first frame has no previous start to measure a period from.
        if (previousStartNs != 0) {
            frameSamples.push_back(
                { slotFrame[slot], static_cast<double>(endNs - startNs) / 1e6,
                  static_cast<double>(startNs - previousStartNs) / 1e6 });
        }
        previousStartNs = startNs;
        stamped[slot]   = false;
    }
    counts[slot]    = 0;
    slotFrame[slot] = frame;

    glQueryCounter(stamps[slot * 2], GL_TIMESTAMP);
}

void GpuTimer::endFrame() {
    assert(!open);
    const size_t slot = frame % depth;
    glQueryCounter(stamps[slot * 2 + 1], GL_TIMESTAMP);
    stamped[slot] = true;
}

void GpuTimer::begin(const std::string_view name) {
    assert(!open);
    const size_t slot = frame % depth;
    if (counts[slot] >= maxPerFrame) {
        return;
    }
    const size_t i = slot * maxPerFrame + counts[slot]++;
    names[i]       = name;
    glBeginQuery(GL_TIME_ELAPSED, queries[i]);
    open = true;
}

void GpuTimer::end() {
    if (!open) {
        return;
    }
    glEndQuery(GL_TIME_ELAPSED);
    open = false;
}

GpuTimer::Summary GpuTimer::summary(const uint32_t warmupFrames) const {
    Summary             result;
    std::vector<double> perName;
    const auto frames = frame > warmupFrames ? frame - warmupFrames : 0;

    for (const auto& first : samples) {
        if (first.frame <= warmupFrames ||
            std::ranges::any_of(result.passes, [&first](const Stat& s) {
                return s.name == first.name;
            })) {
            continue;
        }
        perName.clear();
        for (const auto& s : samples) {
            if (s.frame > warmupFrames && s.name == first.name) {
                perName.push_back(s.ms);
            }
        }
        result.passes.push_back(
            { .name       = first.name,
              .calls      = static_cast<uint32_t>(perName.size()),
              .medianMs   = median(perName),
              .perFrameMs = frames > 0 ?
                                mean(perName) *
                                    static_cast<double>(perName.size()) /
                                    frames :
                                0. });
    }

    std::vector<double> spans;
    std::vector<double> periods;
    for (const auto& s : frameSamples) {
        if (s.frame > warmupFrames) {
            spans.push_back(s.spanMs);
            periods.push_back(s.periodMs);
        }
    }
    result.frame = { .frames       = static_cast<uint32_t>(spans.size()),
                     .spanMedian   = median(spans),
                     .spanMean     = mean(spans),
                     .periodMedian = median(periods),
                     .periodMean   = mean(periods) };
    return result;
}

}  // namespace sponge::platform::opengl::debug
