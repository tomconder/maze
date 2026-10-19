#pragma once

#include "modeldata.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

namespace sponge::scene {

// A range of one mesh's indices, relative to the mesh, and the depth of the
// clusters in it.
struct IndexRange {
    uint32_t firstIndex{ 0 };
    uint32_t indexCount{ 0 };
    uint32_t depth{ 0 };
};

// A slice of an array: its first element and its count.
struct MeshCut {
    uint32_t first{ 0 };
    uint32_t count{ 0 };
};

// The clusters one view draws, for every mesh of every object.
struct ClusterCut {
    std::vector<IndexRange> ranges;
    // [object][mesh]: the slice of ranges that the mesh draws. A count of 0
    // draws the mesh's LOD0.
    std::vector<std::vector<MeshCut>> meshes;
};

// Appends the cut of one mesh to ranges and returns its slice. A cluster is
// in the cut when passes() is true for its self bounds and false for its
// parent bounds. passes must be monotonic over the hierarchy: true for a
// parent means true for its children. Adjacent clusters of one depth merge
// into one range, so a mesh at LOD0 is one range.
template <typename Passes>
MeshCut appendCut(const std::span<const MeshCluster> clusters,
                  const Passes& passes, std::vector<IndexRange>& ranges) {
    const auto first = static_cast<uint32_t>(ranges.size());
    for (const auto& cluster : clusters) {
        if (!passes(cluster.self) || passes(cluster.parent)) {
            continue;
        }
        if (ranges.size() > first) {
            auto& last = ranges.back();
            if (last.depth == cluster.depth &&
                last.firstIndex + last.indexCount == cluster.firstIndex) {
                last.indexCount += cluster.indexCount;
                continue;
            }
        }
        ranges.push_back(
            { cluster.firstIndex, cluster.indexCount, cluster.depth });
    }
    return { first, static_cast<uint32_t>(ranges.size()) - first };
}

// Screen error of a cluster sphere, in pixels: its error over the distance
// to the near side of the sphere, as in meshoptimizer's clusterlod demo
// (boundsError). It is monotonic over the hierarchy, because a parent sphere
// holds its children and its error is at least theirs. pixelsPerUnit is the
// number of pixels one world unit covers at distance 1: the screen height
// times proj[1][1] / 2.
inline float pixelError(const ClusterBounds& bounds, const glm::vec3& eye,
                        const float pixelsPerUnit, const float zNear) {
    const float distance = glm::distance(bounds.center, eye) - bounds.radius;
    return bounds.error / std::max(distance, zNear) * pixelsPerUnit;
}

}  // namespace sponge::scene
