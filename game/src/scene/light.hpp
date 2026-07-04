#pragma once
#include <glm/glm.hpp>

namespace game::scene {

struct DirectionalLight {
    bool      enabled;
    bool      castShadow;
    glm::vec3 color;
    glm::vec3 direction;
    uint32_t  shadowMapRes;
};

struct PointLight {
    glm::vec3 color;
    glm::vec3 position;
};

class Light {
public:
    Light() = delete;

    static float getAttenuationDistance(int32_t index);
};

}  // namespace game::scene
