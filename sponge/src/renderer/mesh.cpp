#include "mesh.h"
#include <meshoptimizer.h>

namespace sponge::renderer {

void Mesh::optimize() {
    size_t numIndices = indices.size();
    size_t numVertices = vertices.size();

    std::vector<uint32_t> optimalIndices;
    std::vector<Vertex> optimalVertices;
    optimalIndices.resize(numIndices);
    optimalVertices.resize(numVertices);

    std::vector<uint32_t> remap(numIndices);
    size_t optimalVertexCount = meshopt_generateVertexRemap(
        remap.data(), indices.data(), numIndices, vertices.data(), numVertices,
        sizeof(Vertex));

    meshopt_remapIndexBuffer(optimalIndices.data(), indices.data(), numIndices,
                             remap.data());

    meshopt_remapVertexBuffer(optimalVertices.data(), vertices.data(),
                              numVertices, sizeof(Vertex), remap.data());

    meshopt_optimizeVertexCache(optimalIndices.data(), optimalIndices.data(),
                                numIndices, optimalVertexCount);

    meshopt_optimizeOverdraw(optimalIndices.data(), optimalIndices.data(),
                             numIndices, &(optimalVertices[0].position.x),
                             optimalVertexCount, sizeof(Vertex), 1.05F);

    meshopt_optimizeVertexFetch(optimalVertices.data(), optimalIndices.data(),
                                numIndices, optimalVertices.data(),
                                optimalVertexCount, sizeof(Vertex));

    indices.swap(optimalIndices);
    vertices.swap(optimalVertices);
}

}  // namespace sponge::renderer
