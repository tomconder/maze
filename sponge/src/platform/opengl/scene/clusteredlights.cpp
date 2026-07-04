// sponge/src/platform/opengl/scene/clusteredlights.cpp
#include "platform/opengl/scene/clusteredlights.hpp"

#include "platform/opengl/renderer/gl.hpp"

#include <cmath>
#include <limits>
#include <vector>

namespace sponge::platform::opengl::scene {

ClusteredLights::ClusteredLights(const float near, const float far) :
    near(near),
    far(far),
    clusterAABBs(maxClusters),
    lightBuffer(static_cast<std::size_t>(maxLights) * sizeof(PointLightGPU)),
    lightGrid(static_cast<std::size_t>(maxClusters) * sizeof(glm::uvec2)),
    lightIndices(static_cast<std::size_t>(maxClusters) * maxLightsPerCluster *
                 sizeof(uint32_t)),
    clusterAABBsSSBO(static_cast<std::size_t>(maxClusters) *
                     sizeof(ClusterAABB)),
    computeParamsSSBO(sizeof(ComputeParams)),
    assignShader("cluster_assign", "/shaders/glsl/cluster_assign.comp.glsl") {}

void ClusteredLights::buildClusterAABBs(const glm::mat4& projection) {
    const glm::mat4 invProj = glm::inverse(projection);

    for (int z = 0; z < tilesZ; ++z) {
        // Z subdivision: sliceNear_k = clusterNear * pow(far/clusterNear,
        // k/tilesZ). Must match clusterIndex() in clustered.slang. Slice 0
        // extends to the camera near plane so fragments closer than
        // clusterNear (which clamp into slice 0) are still inside its AABB.
        const float sliceNear =
            z == 0 ? near :
                     clusterNear * std::pow(far / clusterNear,
                                            static_cast<float>(z) / tilesZ);
        const float sliceFar =
            clusterNear *
            std::pow(far / clusterNear, static_cast<float>(z + 1) / tilesZ);

        for (int y = 0; y < tilesY; ++y) {
            for (int x = 0; x < tilesX; ++x) {
                const float ndcXMin =
                    2.F * static_cast<float>(x) / tilesX - 1.F;
                const float ndcXMax =
                    2.F * static_cast<float>(x + 1) / tilesX - 1.F;
                const float ndcYMin =
                    2.F * static_cast<float>(y) / tilesY - 1.F;
                const float ndcYMax =
                    2.F * static_cast<float>(y + 1) / tilesY - 1.F;

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

                const int idx = x + y * tilesX + z * tilesX * tilesY;
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
                            static_cast<std::size_t>(maxClusters) *
                                sizeof(ClusterAABB));
}

void ClusteredLights::update(const glm::vec3* positions,
                             const glm::vec3* colors,
                             const int attenuationIndex, const int numLights,
                             const glm::mat4& view,
                             const glm::mat4& projection) {
    if (projection != lastProjection) {
        buildClusterAABBs(projection);
        lastProjection = projection;
    }

    std::vector<PointLightGPU> gpuLights(static_cast<std::size_t>(numLights));
    for (int i = 0; i < numLights; ++i) {
        gpuLights[static_cast<std::size_t>(i)] = {
            .color    = colors[i],
            .pad0     = 0.F,
            .position = positions[i],
            .pad1     = 0.F,
        };
    }
    lightBuffer.update(gpuLights.data(), static_cast<std::size_t>(numLights) *
                                             sizeof(PointLightGPU));

    // glm is column-major: row r of the view matrix is (view[c][r] for c 0..3).
    const ComputeParams params{
        .viewRow0         = { view[0][0], view[1][0], view[2][0], view[3][0] },
        .viewRow1         = { view[0][1], view[1][1], view[2][1], view[3][1] },
        .viewRow2         = { view[0][2], view[1][2], view[2][2], view[3][2] },
        .near             = near,
        .far              = far,
        .numLights        = numLights,
        .numClusters      = maxClusters,
        .attenuationIndex = attenuationIndex,
        .pad              = {},
    };
    computeParamsSSBO.update(&params, sizeof(ComputeParams));

    lightBuffer.bindBase(3);
    lightGrid.bindBase(4);
    lightIndices.bindBase(5);
    clusterAABBsSSBO.bindBase(6);
    computeParamsSSBO.bindBase(7);

    assignShader.dispatch(static_cast<uint32_t>((maxClusters + 63) / 64));

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ClusteredLights::bindSSBOs() const {
    lightBuffer.bindBase(3);
    lightGrid.bindBase(4);
    lightIndices.bindBase(5);
}

}  // namespace sponge::platform::opengl::scene
