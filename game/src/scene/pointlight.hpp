#pragma once
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>

namespace sponge::platform::opengl::scene {
class ShadowMap;
}

enum class LightType { DIRECTIONAL, POINT, SPOT };

class PointLight {
   public:
    PointLight() = default;

    LightType type = LightType::POINT;
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    float fov = glm::radians(45.0f);
    float aspectRatio = 1.0f;
    std::unique_ptr<sponge::platform::opengl::scene::ShadowMap> shadowMap;
    glm::mat4 lightViewMatrix = glm::mat4(1.0f);
    glm::mat4 lightProjectionMatrix = glm::mat4(1.0f);

    glm::vec3 getAttenuation() const {
        return { constant, linear, quadratic };
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

    uint8_t castsShadows = 0;
};
