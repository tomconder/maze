#include "meshopt.hpp"

#include "modeldata.hpp"
#include "vertex.hpp"

#include <clusterlod.h>
#include <meshoptimizer.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace assetconv {
using sponge::scene::ClusterBounds;
using sponge::scene::MeshCluster;
using sponge::scene::PackedVertex;
using sponge::scene::ParsedMesh;
using sponge::scene::Vertex;

namespace {
// x, y, z in 10 bits each and w in 2, in GL_INT_2_10_10_10_REV bit order.
uint32_t packSnorm1010102(const glm::vec4& v) {
    const auto bits = [](const float f, const int n) {
        return static_cast<uint32_t>(meshopt_quantizeSnorm(f, n)) &
               ((1U << n) - 1U);
    };
    return bits(v.x, 10) | (bits(v.y, 10) << 10) | (bits(v.z, 10) << 20) |
           (bits(v.w, 2) << 30);
}
}  // namespace

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

namespace {
// Triangles per cluster: the clodDefaultConfig value for rasterization.
constexpr size_t clusterTriangles = 128;

// The normals steer the simplifier, with the weight the meshoptimizer docs
// suggest for unit normals.
constexpr std::array normalWeights = { 0.5F, 0.5F, 0.5F };

// clusterlod reads the attributes from one packed array, as its demo does:
// the weighted normal first, then the UV.
struct ClusterAttributes {
    glm::vec3 normal;
    glm::vec2 texCoords;
};
// clusterlod simplifies in permissive mode, which may collapse across a
// seam unless the attribute that splits it is protected. Protect all five
// floats: UV seams and hard normal edges stay where they are.
constexpr unsigned int protectedAttributes = 0b11111;

ClusterBounds toBounds(const clodBounds& bounds) {
    return { glm::vec3(bounds.center[0], bounds.center[1], bounds.center[2]),
             bounds.radius, bounds.error };
}

// Locks every vertex on an open edge: an edge that only one triangle uses.
// Edges are counted by position, so a UV seam is not an open edge.
// clusterlod locks the borders between its own groups, but not the border
// of the mesh, and Sponza's meshes meet along those borders. A border
// vertex that moves opens a crack to the next mesh.
std::vector<unsigned char> lockOpenBorders(const ParsedMesh& mesh) {
    std::vector<unsigned int> remap(mesh.vertices.size());
    meshopt_generatePositionRemap(remap.data(), &mesh.vertices[0].position.x,
                                  mesh.vertices.size(), sizeof(Vertex));

    std::unordered_map<uint64_t, uint32_t> edgeUses;
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        for (size_t e = 0; e < 3; e++) {
            const uint32_t a = remap[mesh.indices[i + e]];
            const uint32_t b = remap[mesh.indices[i + ((e + 1) % 3)]];
            edgeUses[(uint64_t{ std::min(a, b) } << 32) | std::max(a, b)]++;
        }
    }

    std::vector<unsigned char> locks(mesh.vertices.size(), 0);
    for (const auto& [edge, uses] : edgeUses) {
        if (uses == 1) {
            locks[edge >> 32]         = meshopt_SimplifyVertex_Lock;
            locks[edge & 0xFFFFFFFFU] = meshopt_SimplifyVertex_Lock;
        }
    }
    // remap points each vertex at the first vertex with its position, so
    // copy the lock to every other vertex at that position.
    for (size_t v = 0; v < locks.size(); v++) {
        locks[v] = locks[remap[v]];
    }
    return locks;
}

// One LOD0 cluster over the whole mesh that no view replaces. Its spheres
// are never read: an error of 0 always passes and FLT_MAX never does.
MeshCluster wholeMesh(const ParsedMesh& mesh) {
    return { .firstIndex = 0,
             .indexCount = static_cast<uint32_t>(mesh.indices.size()),
             .depth      = 0,
             .self       = {},
             .parent     = { .error = FLT_MAX } };
}
}  // namespace

void buildClusters(ParsedMesh& mesh) {
    // A cutout or blended surface changes shape through its texture alpha
    // when simplified, so those meshes keep LOD0 only.
    if (mesh.alphaMode != sponge::scene::AlphaMode::Opaque ||
        mesh.indices.empty()) {
        mesh.clusters = { wholeMesh(mesh) };
        return;
    }

    std::vector<ClusterAttributes> attributes(mesh.vertices.size());
    std::ranges::transform(
        mesh.vertices, attributes.begin(), [](const Vertex& v) {
            return ClusterAttributes{ v.normal, v.texCoords };
        });
    const auto locks = lockOpenBorders(mesh);

    clodMesh input{};
    input.indices                  = mesh.indices.data();
    input.index_count              = mesh.indices.size();
    input.vertex_count             = mesh.vertices.size();
    input.vertex_positions         = &mesh.vertices[0].position.x;
    input.vertex_positions_stride  = sizeof(Vertex);
    input.vertex_attributes        = &attributes[0].normal.x;
    input.vertex_attributes_stride = sizeof(ClusterAttributes);
    input.vertex_lock              = locks.data();
    input.attribute_weights        = normalWeights.data();
    input.attribute_count          = normalWeights.size();
    input.attribute_protect_mask   = protectedAttributes;

    std::vector<uint32_t>    indices;
    std::vector<MeshCluster> clusters;
    // clodGroup::simplified of each group, by the id the callback returns.
    std::vector<clodBounds> groups;
    clodBuild(
        clodDefaultConfig(clusterTriangles), input,
        [&](const clodGroup group, const clodCluster* groupClusters,
            const size_t count) -> int {
            for (size_t i = 0; i < count; i++) {
                const auto& cluster = groupClusters[i];
                clusters.push_back({
                    .firstIndex = static_cast<uint32_t>(indices.size()),
                    .indexCount = static_cast<uint32_t>(cluster.index_count),
                    .depth      = static_cast<uint32_t>(group.depth),
                    // A simplified cluster has the error of the group
                    // it was made from. LOD0 has none.
                    .self   = cluster.refined < 0 ?
                                  ClusterBounds{} :
                                  toBounds(groups[cluster.refined]),
                    .parent = toBounds(group.simplified),
                });
                indices.insert(indices.end(), cluster.indices,
                               cluster.indices + cluster.index_count);
            }
            groups.push_back(group.simplified);
            return static_cast<int>(groups.size() - 1);
        });

    mesh.indices  = std::move(indices);
    mesh.clusters = std::move(clusters);

    // clodBuild wrote the triangles in a new order, so the fetch order from
    // optimizeMesh() no longer holds. Only index values change, not their
    // positions, so the cluster ranges stay valid.
    const auto fetchVertexCount = meshopt_optimizeVertexFetch(
        mesh.vertices.data(), mesh.indices.data(), mesh.indices.size(),
        mesh.vertices.data(), mesh.vertices.size(), sizeof(Vertex));
    mesh.vertices.resize(fetchVertexCount);
}

void packVertices(ParsedMesh& mesh) {
    mesh.packedVertices.resize(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        const auto& v          = mesh.vertices[i];
        mesh.packedVertices[i] = {
            .position  = v.position,
            .texCoords = v.texCoords,
            .normal    = packSnorm1010102(glm::vec4(v.normal, 0.F)),
            .tangent   = packSnorm1010102(v.tangent),
        };
    }
    std::vector<Vertex>().swap(mesh.vertices);
}

}  // namespace assetconv
