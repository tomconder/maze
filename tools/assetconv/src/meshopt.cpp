#include "meshopt.hpp"

#include "modeldata.hpp"
#include "vertex.hpp"

#include <meshoptimizer.h>

#include <cstdint>
#include <vector>

namespace assetconv {
using sponge::scene::ParsedMesh;
using sponge::scene::Vertex;

void optimizeMesh(ParsedMesh& mesh) {
    const auto indexCount  = mesh.indices.size();
    const auto vertexCount = mesh.vertices.size();
    if (indexCount == 0 || vertexCount == 0) {
        return;
    }

    std::vector<uint32_t> remap(indexCount);
    const auto            optimalVertexCount = meshopt_generateVertexRemap(
        remap.data(), mesh.indices.data(), indexCount, mesh.vertices.data(),
        vertexCount, sizeof(Vertex));

    std::vector<uint32_t> optimalIndices(indexCount);
    std::vector<Vertex>   optimalVertices(vertexCount);

    meshopt_remapIndexBuffer(optimalIndices.data(), mesh.indices.data(),
                             indexCount, remap.data());

    meshopt_remapVertexBuffer(optimalVertices.data(), mesh.vertices.data(),
                              vertexCount, sizeof(Vertex), remap.data());

    meshopt_optimizeVertexCache(optimalIndices.data(), optimalIndices.data(),
                                indexCount, optimalVertexCount);

    meshopt_optimizeOverdraw(optimalIndices.data(), optimalIndices.data(),
                             indexCount, &optimalVertices[0].position.x,
                             optimalVertexCount, sizeof(Vertex), 1.05F);

    const auto fetchVertexCount = meshopt_optimizeVertexFetch(
        optimalVertices.data(), optimalIndices.data(), indexCount,
        optimalVertices.data(), optimalVertexCount, sizeof(Vertex));

    // optimizeVertexFetch returns the count actually referenced, which can be
    // lower than the remapped count; the tail is unused and must not ship.
    optimalVertices.resize(fetchVertexCount);

    mesh.indices  = std::move(optimalIndices);
    mesh.vertices = std::move(optimalVertices);
}

}  // namespace assetconv
