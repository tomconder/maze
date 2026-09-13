#pragma once

#include "scene/mesh.hpp"

#include <cstdint>
#include <vector>

namespace assetconv {

// Per-triangle tangent accumulation (Lengyel's method), averaged per vertex
// and Gram-Schmidt orthogonalized against the vertex normal.
void computeTangents(std::vector<sponge::scene::Vertex>& vertices,
                     const std::vector<uint32_t>&        indices);

}  // namespace assetconv
