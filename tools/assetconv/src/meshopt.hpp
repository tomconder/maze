#pragma once

#include "modeldata.hpp"

// Vertex cache, overdraw and fetch optimization. This used to run in the
// engine on every load; it is deterministic and depends only on the mesh, so
// the baked file carries the result and meshoptimizer stays out of the game.
namespace assetconv {

void optimizeMesh(sponge::scene::ParsedMesh& mesh);

}  // namespace assetconv
