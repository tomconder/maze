#pragma once
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class PointLight {
   public:
    PointLight() = default;
    PointLight(const glm::vec3& position, const glm::vec3& translation,
               int32_t attenuationIndex);

    glm::vec3 getPosition() const {
        return position;
    }

    glm::vec3 getTranslation() const {
        return translation;
    }

    glm::vec4 getAttenuationFromIndex(int32_t index) const;

    glm::vec3 getAttenuation() const {
        return { constant, linear, quadratic };
    }

    void setAttenuationFromIndex(int32_t index);
    void setPosition(const glm::vec3& position);
    void setTranslation(const glm::vec3& translation);

   private:
    glm::vec3 position;
    glm::vec3 translation;

    float distance;
    float constant;
    float linear;
    float quadratic;

    int32_t attenuationIndex = 4;
};
