#include "scene/light.hpp"

#include <algorithm>

namespace game::scene {

constexpr float distance[11] = { 7.F,   13.F,  20.F,  32.F,  50.F, 65.F,
                                 100.F, 160.F, 200.F, 325.F, 600.F };

float Light::getAttenuationDistance(const int32_t index) {
    const uint32_t value = std::clamp(index, 0, 10);
    return distance[value];
}
}  // namespace game::scene
