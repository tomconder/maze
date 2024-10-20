#pragma once

#include <chrono>

namespace sponge::core {

constexpr double MICROSECONDS_TO_SECONDS = 1e-6F;

using std::chrono::high_resolution_clock;

class Timer {
   public:
    Timer() = default;

    void tick() {
        const auto currentTicks{ high_resolution_clock::now() };
        const auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(
                currentTicks - previousTicks);
        elapsedSeconds = duration.count() * MICROSECONDS_TO_SECONDS;
        previousTicks = currentTicks;
    }

    double getElapsedSeconds() const {
        return elapsedSeconds;
    }

   private:
    double elapsedSeconds{ 0.0 };
    high_resolution_clock::time_point previousTicks{
        high_resolution_clock::now()
    };
};

}  // namespace sponge::core
