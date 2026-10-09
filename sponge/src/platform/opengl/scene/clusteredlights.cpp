// sponge/src/platform/opengl/scene/clusteredlights.cpp
#include "platform/opengl/scene/clusteredlights.hpp"

#include "platform/opengl/renderer/gl.hpp"

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
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
    assignShader(renderer::ShaderCreateInfo{
        .name          = "cluster_assign",
        .computeShader = "cluster_assign.comp",
    }) {}

void ClusteredLights::buildClusterAABBs(const glm::mat4& projection) {
    const glm::mat4 invProj = glm::inverse(projection);

    // Unit view-space direction through each tile corner. The (tilesX + 1) x
    // (tilesY + 1) grid is the same for every z-slice and shared by
    // neighbouring tiles, so unproject it once.
    constexpr int          cornersX = tilesX + 1;
    constexpr int          cornersY = tilesY + 1;
    std::vector<glm::vec3> cornerDirs(static_cast<std::size_t>(cornersX) *
                                      cornersY);
    for (int y = 0; y < cornersY; ++y) {
        for (int x = 0; x < cornersX; ++x) {
            const glm::vec4 ndc{ 2.F * static_cast<float>(x) / tilesX - 1.F,
                                 2.F * static_cast<float>(y) / tilesY - 1.F,
                                 -1.F, 1.F };
            auto            v = invProj * ndc;
            v /= v.w;
            cornerDirs[static_cast<std::size_t>(x + y * cornersX)] =
                glm::normalize(glm::vec3(v));
        }
    }

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
                const auto corner = [&](const int cx, const int cy) {
                    return cornerDirs[static_cast<std::size_t>(cx +
                                                               cy * cornersX)];
                };
                const glm::vec3 dir00 = corner(x, y);
                const glm::vec3 dir10 = corner(x + 1, y);
                const glm::vec3 dir01 = corner(x, y + 1);
                const glm::vec3 dir11 = corner(x + 1, y + 1);

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
    // lightBuffer holds maxLights entries; a larger count would overrun it.
    const int count = std::clamp(numLights, 0, maxLights);

    if (projection != lastProjection) {
        buildClusterAABBs(projection);
        lastProjection = projection;
    }

    std::vector<PointLightGPU> gpuLights(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        gpuLights[static_cast<std::size_t>(i)] = {
            .color    = colors[i],
            .pad0     = 0.F,
            .position = positions[i],
            .pad1     = 0.F,
        };
    }
    lightBuffer.update(gpuLights.data(),
                       static_cast<std::size_t>(count) * sizeof(PointLightGPU));

    // glm is column-major: row r of the view matrix is (view[c][r] for c 0..3).
    const ComputeParams params{
        .viewRow0         = { view[0][0], view[1][0], view[2][0], view[3][0] },
        .viewRow1         = { view[0][1], view[1][1], view[2][1], view[3][1] },
        .viewRow2         = { view[0][2], view[1][2], view[2][2], view[3][2] },
        .near             = near,
        .far              = far,
        .numLights        = count,
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
