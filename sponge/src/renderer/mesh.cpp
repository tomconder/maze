#include "mesh.hpp"
#include <meshoptimizer.h>

namespace sponge::renderer {

void Mesh::optimize() {
    std::vector<uint32_t> remap(indices.size());
    const auto optimalVertexCount = meshopt_generateVertexRemap(
        remap.data(), indices.data(), indices.size(), vertices.data(),
        vertices.size(), sizeof(Vertex));

    std::vector<uint32_t> optimalIndices;
    std::vector<Vertex> optimalVertices;
    optimalIndices.resize(indices.size());
    optimalVertices.resize(vertices.size());

    meshopt_remapIndexBuffer(optimalIndices.data(), indices.data(),
                             indices.size(), remap.data());

    meshopt_remapVertexBuffer(optimalVertices.data(), vertices.data(),
                              vertices.size(), sizeof(Vertex), remap.data());

    meshopt_optimizeVertexCache(optimalIndices.data(), optimalIndices.data(),
                                indices.size(), optimalVertexCount);

    meshopt_optimizeOverdraw(optimalIndices.data(), optimalIndices.data(),
                             indices.size(), &optimalVertices[0].position.x,
                             optimalVertexCount, sizeof(Vertex), 1.05F);

    meshopt_optimizeVertexFetch(optimalVertices.data(), optimalIndices.data(),
                                indices.size(), optimalVertices.data(),
                                optimalVertexCount, sizeof(Vertex));

    indices.assign(optimalIndices.begin(), optimalIndices.end());
    vertices.assign(optimalVertices.begin(), optimalVertices.end());
}

}  // namespace sponge::renderer
