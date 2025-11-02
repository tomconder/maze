#pragma once
#include <glm/glm.hpp>

namespace game::scene {

struct DirectionalLight {
    bool enabled;
    bool castShadow;
    glm::vec3 color;
    glm::vec3 direction;
    float shadowBias;
    uint32_t shadowMapRes;
};

struct PointLight {
    glm::vec3 color;
    glm::vec3 position;

    float distance;
    float constant;
    float linear;
    float quadratic;
};

class Light {
public:
    Light() = default;

    static glm::vec4 getAttenuationFromIndex(int32_t index);
};

}  // namespace game::scene
