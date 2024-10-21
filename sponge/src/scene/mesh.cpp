#include "mesh.hpp"
#include <meshoptimizer.h>

namespace sponge::scene {

void Mesh::optimize() {
    std::vector<uint32_t> remap(numIndices);
    const auto optimalVertexCount = meshopt_generateVertexRemap(
        remap.data(), indices.data(), numIndices, vertices.data(), numVertices,
        sizeof(Vertex));

    std::vector<uint32_t> optimalIndices;
    std::vector<Vertex> optimalVertices;
    optimalIndices.resize(numIndices);
    optimalVertices.resize(numVertices);

    meshopt_remapIndexBuffer(optimalIndices.data(), indices.data(), numIndices,
                             remap.data());

    meshopt_remapVertexBuffer(optimalVertices.data(), vertices.data(),
                              numVertices, sizeof(Vertex), remap.data());

    meshopt_optimizeVertexCache(optimalIndices.data(), optimalIndices.data(),
                                numIndices, optimalVertexCount);

    meshopt_optimizeOverdraw(optimalIndices.data(), optimalIndices.data(),
                             numIndices, &optimalVertices[0].position.x,
                             optimalVertexCount, sizeof(Vertex), 1.05F);

    meshopt_optimizeVertexFetch(optimalVertices.data(), optimalIndices.data(),
                                numIndices, optimalVertices.data(),
                                optimalVertexCount, sizeof(Vertex));

    indices.assign(optimalIndices.begin(), optimalIndices.end());
    vertices.assign(optimalVertices.begin(), optimalVertices.end());
    numVertices = optimalVertexCount;
}

}  // namespace sponge::scene
