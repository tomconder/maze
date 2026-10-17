#pragma once

#include <chrono>

namespace sponge::platform::windows::core {

class WinTimer {
public:
    // Sleeps until target on a high resolution timer, which wakes within about
    // a millisecond. std::this_thread::sleep_until wakes on the 15.6 ms system
    // tick, so a 60 fps cap ran at 32 and a 144 cap at 64. The timer needs
    // Windows 10 version 1803 or later; without it this returns at once. One
    // thread only: the timer is made once and lives as long as the process.
    static void sleepUntil(std::chrono::steady_clock::time_point target);
};

}  // namespace sponge::platform::windows::core
