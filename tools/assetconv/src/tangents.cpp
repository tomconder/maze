#include "tangents.hpp"

#include "vertex.hpp"

#include <meshoptimizer.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace assetconv {
using sponge::scene::Vertex;

void computeTangents(std::vector<Vertex>&   vertices,
                     std::vector<uint32_t>& indices) {
    if (indices.empty() || vertices.empty()) {
        return;
    }

    std::vector<float> tangents(indices.size() * 4);
    meshopt_generateTangents(tangents.data(), indices.data(), indices.size(),
                             &vertices[0].position.x, vertices.size(),
                             sizeof(Vertex), &vertices[0].normal.x,
                             sizeof(Vertex), &vertices[0].texCoords.x,
                             sizeof(Vertex), meshopt_TangentCompatible);

    std::vector<Vertex> corners(indices.size());
    for (size_t i = 0; i < indices.size(); i++) {
        corners[i]         = vertices[indices[i]];
        corners[i].tangent = { tangents[i * 4], tangents[i * 4 + 1],
                               tangents[i * 4 + 2], tangents[i * 4 + 3] };
        indices[i]         = static_cast<uint32_t>(i);
    }
    vertices = std::move(corners);
}

}  // namespace assetconv
