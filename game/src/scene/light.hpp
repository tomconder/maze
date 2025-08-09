#pragma once
#include <glm/glm.hpp>

namespace game::scene {
class Light {
public:
    Light() = default;

    float getAttenuationConstant() const {
        return constant;
    }

    float getAttenuationLinear() const {
        return linear;
    }

    float getAttenuationQuadratic() const {
        return quadratic;
    }

    static glm::vec4 getAttenuationFromIndex(int32_t index);
    void setAttenuationFromIndex(int32_t index);

    glm::vec3 position;
    glm::vec3 translation;
    glm::vec3 color;

    float distance;
    float constant;
    float linear;
    float quadratic;
};
}  // namespace game::scene
