#include "scene/tangents.hpp"

#include "scene/mesh.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sponge::scene {

// Per-triangle tangent accumulation (Lengyel's method), averaged per vertex
// and Gram-Schmidt orthogonalized against the vertex normal.
void computeTangents(std::vector<Vertex>&         vertices,
                     const std::vector<uint32_t>& indices) {
    std::vector tan(vertices.size(), glm::vec3(0.F));
    std::vector bitan(vertices.size(), glm::vec3(0.F));

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const auto i0 = indices[i];
        const auto i1 = indices[i + 1];
        const auto i2 = indices[i + 2];

        const auto& v0 = vertices[i0];
        const auto& v1 = vertices[i1];
        const auto& v2 = vertices[i2];

        const auto edge1 = v1.position - v0.position;
        const auto edge2 = v2.position - v0.position;
        const auto duv1  = v1.texCoords - v0.texCoords;
        const auto duv2  = v2.texCoords - v0.texCoords;

        const auto det = duv1.x * duv2.y - duv2.x * duv1.y;
        if (glm::abs(det) < 1e-8F) {
            continue;
        }
        const auto f = 1.F / det;

        const auto tangent   = f * (duv2.y * edge1 - duv1.y * edge2);
        const auto bitangent = f * (duv1.x * edge2 - duv2.x * edge1);

        for (const auto idx : { i0, i1, i2 }) {
            tan[idx] += tangent;
            bitan[idx] += bitangent;
        }
    }

    for (size_t i = 0; i < vertices.size(); i++) {
        const auto& n = vertices[i].normal;
        auto        t = tan[i] - n * dot(n, tan[i]);
        if (dot(t, t) < 1e-12F) {
            // degenerate UVs; fall back to any vector orthogonal to normal
            t = glm::abs(n.x) > glm::abs(n.z) ? glm::vec3(-n.y, n.x, 0.F) :
                                                glm::vec3(0.F, -n.z, n.y);
        }
        t                   = normalize(t);
        const auto sign     = dot(cross(n, t), bitan[i]) < 0.F ? -1.F : 1.F;
        vertices[i].tangent = glm::vec4(t, sign);
    }
}

}  // namespace sponge::scene
