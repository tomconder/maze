#pragma once

#include "vertex.hpp"

#include <cstdint>
#include <vector>

namespace assetconv {

// MikkTSpace-compatible tangents, which glTF requires when a file has none
// and which normal maps are usually baked against. Tangents are per corner,
// so this unwelds the mesh: one vertex per index, indices 0..n-1.
// optimizeMesh welds it again, leaving vertices split only where the
// tangents differ, e.g. on UV mirror seams.
void computeTangents(std::vector<sponge::scene::Vertex>& vertices,
                     std::vector<uint32_t>&              indices);

}  // namespace assetconv
