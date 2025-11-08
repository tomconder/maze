#pragma once
#include <glm/glm.hpp>

namespace game::scene {

struct DirectionalLight {
    bool      enabled;
    bool      castShadow;
    glm::vec3 color;
    glm::vec3 direction;
    float     shadowBias;
    uint32_t  shadowMapRes;
};

struct PointLight {
    glm::vec3 color;
    glm::vec3 position;
    uint32_t  attenuationIndex;
};

class Light {
public:
    Light() = delete;

    static float getAttenuationDistance(int32_t index);
};

}  // namespace game::scene
