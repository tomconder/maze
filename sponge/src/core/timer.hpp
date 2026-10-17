#pragma once

#include <chrono>

namespace sponge::core {

class Timer {
public:
    void tick() {
        const auto now = std::chrono::steady_clock::now();
        elapsedSeconds =
            std::chrono::duration<double>(now - previousTicks).count();
        previousTicks = now;
    }

    double getElapsedSeconds() const {
        return elapsedSeconds;
    }

    // Sleeps the calling thread until target. Windows has its own timer for
    // this, because sleep_until wakes on the 15.6 ms system tick there.
    static void sleepUntil(std::chrono::steady_clock::time_point target);

private:
    double                                elapsedSeconds{ 0.0 };
    std::chrono::steady_clock::time_point previousTicks{
        std::chrono::steady_clock::now(),
    };
};

}  // namespace sponge::core
