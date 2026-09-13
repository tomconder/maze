#pragma once

#include "scene/mesh.hpp"

#include <cstdint>
#include <vector>

namespace sponge::scene {

// Per-triangle tangent accumulation (Lengyel's method), averaged per vertex
// and Gram-Schmidt orthogonalized against the vertex normal.
void computeTangents(std::vector<Vertex>&         vertices,
                     const std::vector<uint32_t>& indices);

}  // namespace sponge::scene
