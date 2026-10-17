#include "platform/windows/core/wintimer.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <chrono>

namespace sponge::platform::windows::core {

void WinTimer::sleepUntil(const std::chrono::steady_clock::time_point target) {
    static HANDLE timer = CreateWaitableTimerExW(
        nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
        TIMER_ALL_ACCESS);
    const auto remaining = target - std::chrono::steady_clock::now();
    if (timer == nullptr ||
        remaining <= std::chrono::steady_clock::duration::zero()) {
        return;
    }

    // Negative is relative, in units of 100 ns.
    LARGE_INTEGER due;
    due.QuadPart =
        -std::chrono::duration_cast<std::chrono::nanoseconds>(remaining)
             .count() /
        100;
    if (SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE) != 0) {
        WaitForSingleObject(timer, INFINITE);
    }
}

}  // namespace sponge::platform::windows::core
