#pragma once

#include <glm/glm.hpp>

namespace game::scene {

// Whole object. A mesh that mixes glass and stone stays opaque.
struct SceneRefraction {
    bool      refractive{ false };
    float     ior{ 1.5F };
    float     thickness{ 0.5F };
    glm::vec3 tint{ 1.F };
};

}  // namespace game::scene
