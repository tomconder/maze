// sponge/src/platform/opengl/scene/clusteredlights.hpp
#pragma once

#include "platform/opengl/renderer/computeshader.hpp"
#include "platform/opengl/renderer/ssbo.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace sponge::platform::opengl::scene {

class ClusteredLights {
public:
    // Cluster grid — must match TILES_X/Y/Z in clustered.slang.
    // ponytail: fixed 16x9x24 grid (Doom 2016 shape); slices are less cubic
    // at narrow FOV, which only coarsens culling, never causes artifacts.
    static constexpr int tilesX      = 16;
    static constexpr int tilesY      = 9;
    static constexpr int tilesZ      = 24;
    static constexpr int maxClusters = tilesX * tilesY * tilesZ;
    // Near bound of the cluster grid; slices below it are wasted on empty
    // space in front of the camera. Slice 0's AABB extends to the camera
    // near plane so closer fragments are still covered.
    static constexpr float clusterNear = 1.F;
    // Max scene light count; frame snapshots and the debug slider use it too.
    static constexpr int maxLights = 128;
    // Must match MAX_LIGHTS_PER_CLUSTER in clustered.slang. Equal to
    // maxLights so per-cluster truncation (visible cluster seams) cannot
    // occur. ~1.7 MB of index storage.
    static constexpr int maxLightsPerCluster = maxLights;

    ClusteredLights(float near, float far);

    // Rebuilds the grid if the projection changed and dispatches the
    // clustered light-culling compute shader.
    void update(const glm::vec3* positions, const glm::vec3* colors,
                int attenuationIndex, int numLights, const glm::mat4& view,
                const glm::mat4& projection);

    void bindSSBOs() const;

private:
    // std430 layout, 32 bytes — must match PointLightGPU in
    // cluster_assign.slang and pbr.slang
    struct PointLightGPU {
        glm::vec3 color;
        float     pad0;
        glm::vec3 position;
        float     pad1;
    };
    static_assert(sizeof(PointLightGPU) == 32);

    // std430-compatible: explicit padding so vec3 → 16-byte slot.
    // Must match ClusterAABB in cluster_assign.comp.glsl.
    struct ClusterAABB {
        glm::vec3 minBounds;
        float     padMin{ 0.F };
        glm::vec3 maxBounds;
        float     padMax{ 0.F };
    };
    static_assert(sizeof(ClusterAABB) == 32);

    // std430 layout, 80 bytes — must match ComputeParams in
    // cluster_assign.slang. The view transform is passed as explicit rows
    // (not a mat4): SSBO matrix layout qualifiers on nested structs are
    // driver-inconsistent, and a transposed read silently breaks culling.
    struct ComputeParams {
        glm::vec4          viewRow0{ 0.F };
        glm::vec4          viewRow1{ 0.F };
        glm::vec4          viewRow2{ 0.F };
        float              near{ 0.F };
        float              far{ 0.F };
        int                numLights{ 0 };
        int                numClusters{ 0 };
        int                attenuationIndex{ 0 };
        std::array<int, 3> pad{};
    };
    static_assert(sizeof(ComputeParams) == 80);

    // Precomputes view-space AABBs for all clusters from the projection matrix.
    // Called once per unique projection (e.g., on FOV/resize change).
    // Z subdivision: sliceNear_k = near * pow(far/near, k/tilesZ).
    // Must match clusterIndex() in clustered.slang.
    void buildClusterAABBs(const glm::mat4& projection);

    float     near;
    float     far;
    glm::mat4 lastProjection{ 0.F };

    std::vector<ClusterAABB> clusterAABBs;

    renderer::SSBO lightBuffer;  // binding 3 — PointLightGPU[]
    renderer::SSBO lightGrid;  // binding 4 — uvec2[] (base, count) per cluster
    renderer::SSBO lightIndices;  // binding 5 — uint[] light index list
    renderer::SSBO
        clusterAABBsSSBO;  // binding 6 — ClusterAABB[] (compute input)
    renderer::SSBO computeParamsSSBO;  // binding 7 — ComputeParams

    renderer::ComputeShader assignShader;
};

}  // namespace sponge::platform::opengl::scene
