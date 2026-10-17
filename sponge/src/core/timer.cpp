#include "core/timer.hpp"

#include <chrono>
#include <thread>

#if _WIN32 || WIN32
#include "platform/windows/core/wintimer.hpp"
#endif

namespace sponge::core {

void Timer::sleepUntil(const std::chrono::steady_clock::time_point target) {
#if _WIN32 || WIN32
    platform::windows::core::WinTimer::sleepUntil(target);
#else
    std::this_thread::sleep_until(target);
#endif
}

}  // namespace sponge::core
