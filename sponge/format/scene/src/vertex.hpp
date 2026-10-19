#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace sponge::scene {
struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoords;
    glm::vec3 normal;
    // xyz = tangent, w = bitangent handedness sign (glTF convention)
    glm::vec4 tangent{ 0.F, 0.F, 0.F, 1.F };
};

// What a baked model stores and what the GPU reads: 28 bytes, not 48.
// normal and tangent are signed normalized 10:10:10:2 integers
// (GL_INT_2_10_10_10_REV). The GPU turns them back into floats on fetch,
// so the shaders still read float3 / float4. Positions and UVs stay float:
// Sponza's UVs reach 32, where a half float is off by 1/64 of a tile.
struct PackedVertex {
    glm::vec3 position;
    glm::vec2 texCoords;
    uint32_t  normal;   // xyz; w unused
    uint32_t  tangent;  // xyz; w = bitangent sign, -1 or 1
};
}  // namespace sponge::scene
