#include "pointlight.hpp"

namespace {
struct LightAttenuation {
    float distance;
    float constant;
    float linear;
    float quadratic;
};

// Attenuation intensity; see https://learnopengl.com/Lighting/Light-casters
constexpr LightAttenuation lightAttenuation[] = {
    { .distance = 7.F, .constant = 1.F, .linear = 0.7F, .quadratic = 1.8F },
    { .distance = 13.F, .constant = 1.F, .linear = 0.35F, .quadratic = 0.44F },
    { .distance = 20.F, .constant = 1.F, .linear = 0.22F, .quadratic = 0.2F },
    { .distance = 32.F, .constant = 1.F, .linear = 0.14F, .quadratic = 0.07F },
    { .distance = 50.F, .constant = 1.F, .linear = 0.09F, .quadratic = 0.032F },
    { .distance = 65.F, .constant = 1.F, .linear = 0.07F, .quadratic = 0.017F },
    { .distance = 100.F,
      .constant = 1.F,
      .linear = 0.045F,
      .quadratic = 0.0075F },
    { .distance = 160.F,
      .constant = 1.F,
      .linear = 0.027F,
      .quadratic = 0.0028F },
    { .distance = 200.F,
      .constant = 1.F,
      .linear = 0.022F,
      .quadratic = 0.0019F },
    { .distance = 325.F,
      .constant = 1.F,
      .linear = 0.014F,
      .quadratic = 0.0007F },
    { .distance = 600.F,
      .constant = 1.F,
      .linear = 0.007F,
      .quadratic = 0.0002F }
};
}  // namespace

PointLight::PointLight(const glm::vec3& position, const glm::vec3& translation,
                       int32_t index) {
    this->position = position;
    this->translation = translation;
    distance = lightAttenuation[index].distance;
    constant = lightAttenuation[index].constant;
    linear = lightAttenuation[index].linear;
    quadratic = lightAttenuation[index].quadratic;
}

void PointLight::setAttenuationFromIndex(int32_t index) {
    distance = lightAttenuation[index].distance;
    constant = lightAttenuation[index].constant;
    linear = lightAttenuation[index].linear;
    quadratic = lightAttenuation[index].quadratic;
}

glm::vec4 PointLight::getAttenuationFromIndex(int32_t index) const {
    return { lightAttenuation[index].distance, lightAttenuation[index].constant,
             lightAttenuation[index].linear,
             lightAttenuation[index].quadratic };
}
