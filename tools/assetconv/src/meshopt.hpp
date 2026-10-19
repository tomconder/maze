#pragma once

#include "modeldata.hpp"

// Vertex cache, overdraw and fetch optimization. This used to run in the
// engine on every load; it is deterministic and depends only on the mesh, so
// the baked file carries the result and meshoptimizer stays out of the game.
namespace assetconv {

void optimizeMesh(sponge::scene::ParsedMesh& mesh);

// Replaces mesh.indices with a cluster LOD hierarchy and fills
// mesh.clusters. Run after optimizeMesh(): the clusters index the final
// vertex order. A mesh that is not opaque gets one LOD0 cluster.
void buildClusters(sponge::scene::ParsedMesh& mesh);

// Quantizes the normals and tangents into mesh.packedVertices and frees the
// float vertices. Run last: simplification needs the float normals.
void packVertices(sponge::scene::ParsedMesh& mesh);

}  // namespace assetconv
