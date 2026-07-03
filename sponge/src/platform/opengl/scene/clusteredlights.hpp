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
    // Screen tile grid — must match TILES_X/Y in clustered.slang. The
    // z-slice count is runtime, derived from the FOV so tiles are roughly
    // cubic (van Oosten eq. 8.2); buffers are sized for maxTilesZ.
    static constexpr int tilesX      = 16;
    static constexpr int tilesY      = 9;
    static constexpr int maxTilesZ   = 96;  // FOV 30° needs 80 slices
    static constexpr int maxClusters = tilesX * tilesY * maxTilesZ;
    // Near bound of the cluster grid; slices below it are wasted on empty
    // space in front of the camera. Slice 0's AABB extends to the camera
    // near plane so closer fragments are still covered.
    static constexpr float clusterNear = 1.F;
    // Must match MAX_LIGHTS_PER_CLUSTER in clustered.slang. Sized >= the max
    // scene light count so per-cluster truncation (visible cluster seams)
    // cannot occur. ~28 MB of index storage at 512 x maxClusters.
    static constexpr int maxLightsPerCluster = 512;

    ClusteredLights(float near, float far);

    // Rebuilds the grid if the projection changed and clears the active-tile
    // flags. Call once per frame before the depth prepass writes tile flags.
    void prepare(const glm::mat4& projection);

    // Dispatches the clustered light-culling compute shader.
    void update(const glm::vec3* positions, const glm::vec3* colors,
                const int* attenuationIndices, int numLights,
                const glm::mat4& view);

    void bindSSBOs() const;

    int getTilesZ() const {
        return tilesZ;
    }

private:
    // std430 layout, 48 bytes — must match PointLightGPU in
    // cluster_assign.comp.glsl
    struct PointLightGPU {
        glm::vec3            color;
        float                pad0;
        glm::vec3            position;
        int                  attenuationIndex;
        std::array<float, 4> pad1{};
    };
    static_assert(sizeof(PointLightGPU) == 48);

    // std430-compatible: explicit padding so vec3 → 16-byte slot.
    // Must match ClusterAABB in cluster_assign.comp.glsl.
    struct ClusterAABB {
        glm::vec3 minBounds;
        float     padMin{ 0.F };
        glm::vec3 maxBounds;
        float     padMax{ 0.F };
    };
    static_assert(sizeof(ClusterAABB) == 32);

    // std430 layout, 64 bytes — must match ComputeParams in
    // cluster_assign.slang. The view transform is passed as explicit rows
    // (not a mat4): SSBO matrix layout qualifiers on nested structs are
    // driver-inconsistent, and a transposed read silently breaks culling.
    struct ComputeParams {
        glm::vec4 viewRow0{ 0.F };
        glm::vec4 viewRow1{ 0.F };
        glm::vec4 viewRow2{ 0.F };
        float     near{ 0.F };
        float     far{ 0.F };
        int       numLights{ 0 };
        int       numClusters{ 0 };
    };
    static_assert(sizeof(ComputeParams) == 64);

    // Precomputes view-space AABBs for all clusters from the projection matrix.
    // Called once per unique projection (e.g., on FOV/resize change).
    // Z subdivision: sliceNear_k = near * pow(far/near, k/kTilesZ).
    // Must match clusterIndex() in clustered.slang and the GLSL compute shader.
    void buildClusterAABBs(const glm::mat4& projection);

    float     near;
    float     far;
    int       tilesZ      = 1;
    int       numClusters = tilesX * tilesY;
    glm::mat4 lastProjection{ 0.F };

    std::vector<ClusterAABB> clusterAABBs;

    renderer::SSBO lightBuffer;  // binding 3 — PointLightGPU[]
    renderer::SSBO lightGrid;  // binding 4 — uvec2[] (base, count) per cluster
    renderer::SSBO lightIndices;  // binding 5 — uint[] light index list
    renderer::SSBO
        clusterAABBsSSBO;  // binding 6 — ClusterAABB[] (compute input)
    renderer::SSBO computeParamsSSBO;  // binding 7 — ComputeParams
    renderer::SSBO tileFlags;          // binding 8 — uint[] active-tile flags

    renderer::ComputeShader assignShader;
};

}  // namespace sponge::platform::opengl::scene
