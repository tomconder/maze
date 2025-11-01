#pragma once
#include <glm/glm.hpp>

namespace game::scene {

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 color;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 translation;

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
