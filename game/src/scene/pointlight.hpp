#pragma once
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class PointLight {
   public:
    PointLight() = default;

    glm::vec3 getAttenuation() const {
        return { constant, linear, quadratic };
    }

    glm::vec4 getAttenuationFromIndex(int32_t index) const;
    void setAttenuationFromIndex(int32_t index);

    glm::vec3 position;
    glm::vec3 translation;

    float distance;
    float constant;
    float linear;
    float quadratic;
};
