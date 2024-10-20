#pragma once

#include <chrono>

constexpr double MICROSECONDS_TO_SECONDS = 1e-6F;

namespace sponge::core {

class Timer {
   public:
    void tick() {
        auto currentTicks{ std::chrono::high_resolution_clock::now() };
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            currentTicks - previousTicks);
        elapsedSeconds = duration.count() * MICROSECONDS_TO_SECONDS;
        previousTicks = currentTicks;
    }

    double getElapsedSeconds() const {
        return elapsedSeconds;
    }

   private:
    double elapsedSeconds{ 0.0 };
    std::chrono::high_resolution_clock::time_point previousTicks{
        std::chrono::high_resolution_clock::now()
    };
};

}  // namespace sponge::core
