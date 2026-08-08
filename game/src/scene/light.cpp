#include "scene/light.hpp"

#include <algorithm>
#include <array>

namespace game::scene {

// Must match attenuationRadius in lighting.slang.
constexpr std::array<float, 11> distance = {
    3.F, 6.F, 9.F, 16.F, 23.F, 31.F, 47.F, 78.F, 94.F, 155.F, 290.F,
};

float Light::getAttenuationDistance(const int32_t index) {
    const uint32_t value = std::clamp(index, 0, 10);
    return distance[value];
}
}  // namespace game::scene
