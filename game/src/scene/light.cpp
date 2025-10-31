#include "scene/light.hpp"

namespace game::scene {

// Attenuation intensity; see https://learnopengl.com/Lighting/Light-casters
constexpr float lightAttenuationData[][4] = {
    { 7.F, 1.F, 0.7F, 1.8F },        { 13.F, 1.F, 0.35F, 0.44F },
    { 20.F, 1.F, 0.22F, 0.2F },      { 32.F, 1.F, 0.14F, 0.07F },
    { 50.F, 1.F, 0.09F, 0.032F },    { 65.F, 1.F, 0.07F, 0.017F },
    { 100.F, 1.F, 0.045F, 0.0075F }, { 160.F, 1.F, 0.027F, 0.0028F },
    { 200.F, 1.F, 0.022F, 0.0019F }, { 325.F, 1.F, 0.014F, 0.0007F },
    { 600.F, 1.F, 0.007F, 0.0002F }
};

glm::vec4 Light::getAttenuationFromIndex(const int32_t index) {
    const auto* data = lightAttenuationData[index];
    return { data[0], data[1], data[2], data[3] };
}
}  // namespace game::scene
