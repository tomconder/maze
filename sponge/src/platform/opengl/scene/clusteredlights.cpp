// sponge/src/platform/opengl/scene/clusteredlights.cpp
#include "platform/opengl/scene/clusteredlights.hpp"

#include "platform/opengl/renderer/gl.hpp"

#include <cmath>
#include <limits>
#include <vector>

namespace sponge::platform::opengl::scene {

namespace {
constexpr int kMaxLights = 500;
}  // namespace

ClusteredLights::ClusteredLights(const float near, const float far) :
    near(near),
    far(far),
    clusterAABBs(kNumClusters),
    lightBuffer(static_cast<std::size_t>(kMaxLights) * sizeof(PointLightGPU)),
    lightGrid(static_cast<std::size_t>(kNumClusters) * sizeof(glm::uvec2)),
    lightIndices(static_cast<std::size_t>(kNumClusters) * kMaxLightsPerCluster *
                 sizeof(uint32_t)),
    clusterAABBsSSBO(static_cast<std::size_t>(kNumClusters) *
                     sizeof(ClusterAABB)),
    computeParamsSSBO(sizeof(ComputeParams)),
    assignShader("cluster_assign", "/shaders/glsl/cluster_assign.comp.glsl") {}

void ClusteredLights::buildClusterAABBs(const glm::mat4& projection) {
    const glm::mat4 invProj = glm::inverse(projection);

    for (int z = 0; z < kTilesZ; ++z) {
        // Z subdivision: sliceNear_k = near * pow(far/near, k/kTilesZ).
        // Must match clusterIndex() in clustered.slang.
        const float sliceNear =
            near * std::pow(far / near, static_cast<float>(z) / kTilesZ);
        const float sliceFar =
            near * std::pow(far / near, static_cast<float>(z + 1) / kTilesZ);

        for (int y = 0; y < kTilesY; ++y) {
            for (int x = 0; x < kTilesX; ++x) {
                const float ndcXMin =
                    2.F * static_cast<float>(x) / kTilesX - 1.F;
                const float ndcXMax =
                    2.F * static_cast<float>(x + 1) / kTilesX - 1.F;
                const float ndcYMin =
                    2.F * static_cast<float>(y) / kTilesY - 1.F;
                const float ndcYMax =
                    2.F * static_cast<float>(y + 1) / kTilesY - 1.F;

                // Unproject screen corners to unit view-space direction
                // vectors.
                auto unprojectCorner = [&](float nx, float ny) -> glm::vec3 {
                    const glm::vec4 ndc{ nx, ny, -1.F, 1.F };
                    auto            v = invProj * ndc;
                    v /= v.w;
                    return glm::normalize(glm::vec3(v));
                };

                const glm::vec3 dir00 = unprojectCorner(ndcXMin, ndcYMin);
                const glm::vec3 dir10 = unprojectCorner(ndcXMax, ndcYMin);
                const glm::vec3 dir01 = unprojectCorner(ndcXMin, ndcYMax);
                const glm::vec3 dir11 = unprojectCorner(ndcXMax, ndcYMax);

                // Scale each corner direction to the near and far depth planes.
                auto scaleToDepth = [](const glm::vec3& dir,
                                       float            d) -> glm::vec3 {
                    return dir * (d / std::abs(dir.z));
                };

                glm::vec3 minB{ std::numeric_limits<float>::max() };
                glm::vec3 maxB{ -std::numeric_limits<float>::max() };

                for (const auto& dir : { dir00, dir10, dir01, dir11 }) {
                    for (const float d : { sliceNear, sliceFar }) {
                        const glm::vec3 pt = scaleToDepth(dir, d);
                        minB               = glm::min(minB, pt);
                        maxB               = glm::max(maxB, pt);
                    }
                }

                const int idx = x + y * kTilesX + z * kTilesX * kTilesY;
                clusterAABBs[static_cast<std::size_t>(idx)] = {
                    .minBounds = minB,
                    .padMin    = 0.F,
                    .maxBounds = maxB,
                    .padMax    = 0.F,
                };
            }
        }
    }

    clusterAABBsSSBO.update(clusterAABBs.data(),
                            static_cast<std::size_t>(kNumClusters) *
                                sizeof(ClusterAABB));
}

void ClusteredLights::update(const glm::vec3* positions,
                             const glm::vec3* colors,
                             const int* attenuationIndices, const int numLights,
                             const glm::mat4& view,
                             const glm::mat4& projection) {
    if (projection != lastProjection) {
        buildClusterAABBs(projection);
        lastProjection = projection;
    }

    std::vector<PointLightGPU> gpuLights(static_cast<std::size_t>(numLights));
    for (int i = 0; i < numLights; ++i) {
        gpuLights[static_cast<std::size_t>(i)] = {
            .color            = colors[i],
            .pad0             = 0.F,
            .position         = positions[i],
            .attenuationIndex = attenuationIndices[i],
            .pad1             = {},
        };
    }
    lightBuffer.update(gpuLights.data(), static_cast<std::size_t>(numLights) *
                                             sizeof(PointLightGPU));

    const ComputeParams params{
        .view      = view,
        .near      = near,
        .far       = far,
        .numLights = numLights,
        .pad       = 0,
    };
    computeParamsSSBO.update(&params, sizeof(ComputeParams));

    lightBuffer.bindBase(3);
    lightGrid.bindBase(4);
    lightIndices.bindBase(5);
    clusterAABBsSSBO.bindBase(6);
    computeParamsSSBO.bindBase(7);

    // 256 clusters / 64 threads per group = 4 dispatch groups.
    assignShader.dispatch(static_cast<uint32_t>(kNumClusters / 64));

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ClusteredLights::bindSSBOs() const {
    lightBuffer.bindBase(3);
    lightGrid.bindBase(4);
    lightIndices.bindBase(5);
}

}  // namespace sponge::platform::opengl::scene
