#pragma once

namespace sponge {

class Timer {
   public:
    void tick() {
        const uint64_t currentTicks{ SDL_GetPerformanceCounter() };
        const uint64_t delta{ currentTicks - previousTicks };
        previousTicks = currentTicks;
        elapsedSeconds = delta / static_cast<double>(TICKS_PER_SECOND);
    }

    double getElapsedSeconds() const {
        return elapsedSeconds;
    }

   private:
    const uint64_t TICKS_PER_SECOND = SDL_GetPerformanceFrequency();

    double elapsedSeconds{};
    uint64_t previousTicks{};
};

}  // namespace sponge
