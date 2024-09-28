#pragma once

#include <SDL.h>

namespace sponge::core {

class Timer {
   public:
    void tick() {
        uint64_t currentTicks{ SDL_GetPerformanceCounter() };
        const uint64_t frequency{ SDL_GetPerformanceFrequency() };
        if (currentTicks <= previousTicks) {
            currentTicks = previousTicks + 1;
        }
        elapsedSeconds =
            previousTicks > 0
                ? static_cast<double>(currentTicks - previousTicks) / frequency
                : static_cast<double>(1.F / 60.F);
        previousTicks = currentTicks;
    }

    double getElapsedSeconds() const {
        return elapsedSeconds;
    }

   private:
    double elapsedSeconds{};
    uint64_t previousTicks{};
};

}  // namespace sponge::core
