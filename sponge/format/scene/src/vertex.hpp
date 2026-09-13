#pragma once

#include <glm/glm.hpp>

namespace sponge::scene {
struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoords;
    glm::vec3 normal;
    // xyz = tangent, w = bitangent handedness sign (glTF convention)
    glm::vec4 tangent{ 0.F, 0.F, 0.F, 1.F };
};
}  // namespace sponge::scene
