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
    // Grid dimensions — must match TILES_X/Y/Z in clustered.slang and
    // cluster_assign.comp.glsl.
    static constexpr int tilesX      = 16;
    static constexpr int tilesY      = 9;
    static constexpr int tilesZ      = 24;
    static constexpr int numClusters = tilesX * tilesY * tilesZ;
    static constexpr int maxLightsPerCluster =
        128;  // must match MAX_LIGHTS_PER_CLUSTER

    ClusteredLights(float near, float far);

    // Dispatches the clustered light-culling compute shader.
    void update(const glm::vec3* positions, const glm::vec3* colors,
                const int* attenuationIndices, int numLights,
                const glm::mat4& view, const glm::mat4& projection);

    void bindSSBOs() const;

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

    // std430 layout, 80 bytes — must match ComputeParams in
    // cluster_assign.comp.glsl
    struct ComputeParams {
        glm::mat4 view{ 1.F };
        float     near{ 0.F };
        float     far{ 0.F };
        int       numLights{ 0 };
        int       pad{ 0 };
    };
    static_assert(sizeof(ComputeParams) == 80);

    // Precomputes view-space AABBs for all clusters from the projection matrix.
    // Called once per unique projection (e.g., on FOV/resize change).
    // Z subdivision: sliceNear_k = near * pow(far/near, k/kTilesZ).
    // Must match clusterIndex() in clustered.slang and the GLSL compute shader.
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
