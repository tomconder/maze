// cluster_assign.slang: volume tiled forward light culling (reference; see cluster_assign.comp.glsl)
// Note: compiled as hand-written GLSL; slangc emits unsupported extensions on this driver.
#include "include/clustered.slang"

struct PointLightGPU {
    float3 color;
    float  pad0;
    float3 position;  // world-space
    int    attenuationIndex;
    float4 pad1;
};

// std430-compatible: explicit padding so float3 → 16-byte slot.
// Must match ClusterAABB in clusteredlights.hpp.
struct ClusterAABB {
    float3 minBounds;
    float  padMin;
    float3 maxBounds;
    float  padMax;
};

// Must match ComputeParams in clusteredlights.hpp. View passed as explicit
// rows; SSBO matrix layout on nested structs is driver-inconsistent.
struct ComputeParams {
    float4 viewRow0;
    float4 viewRow1;
    float4 viewRow2;
    float  near;
    float  far;
    int    numLights;
    int    numClusters;
};

[[vk::binding(3)]] StructuredBuffer<PointLightGPU> lightBuffer;    // read
[[vk::binding(4)]] RWStructuredBuffer<uint2>       lightGrid;      // write
[[vk::binding(5)]] RWStructuredBuffer<uint>        lightIndices;   // write
[[vk::binding(6)]] StructuredBuffer<ClusterAABB>   clusterAABBs;   // read
[[vk::binding(7)]] StructuredBuffer<ComputeParams> computeParams;  // read
[[vk::binding(8)]] StructuredBuffer<uint>          tileFlags;      // read

// clang-format off
[shader("compute")] [numthreads(64, 1, 1)]
void csMain(uint3 tid : SV_DispatchThreadID) {
    // clang-format on
    int           ci = int(tid.x);
    ComputeParams p  = computeParams[0];
    if (ci >= p.numClusters) {
        return;
    }

    uint base = uint(ci) * uint(MAX_LIGHTS_PER_CLUSTER);

    // Active-tile culling: skip clusters no visible fragment landed in
    // (flags written by the depth prepass).
    if (tileFlags[ci] == 0U) {
        lightGrid[ci] = uint2(base, 0U);
        return;
    }

    ClusterAABB aabb  = clusterAABBs[ci];
    int         count = 0;

    for (int i = 0; i < p.numLights && count < MAX_LIGHTS_PER_CLUSTER; ++i) {
        float4 wp    = float4(lightBuffer[i].position, 1.0);
        float3 vsPos = float3(dot(p.viewRow0, wp), dot(p.viewRow1, wp),
                              dot(p.viewRow2, wp));

        int   aidx = clamp(lightBuffer[i].attenuationIndex, 0, 10);
        float r    = attenuationRadius[aidx];

        float3 closest = clamp(vsPos, aabb.minBounds, aabb.maxBounds);
        if (length(vsPos - closest) <= r) {
            lightIndices[base + uint(count)] = uint(i);
            count++;
        }
    }

    lightGrid[ci] = uint2(base, uint(count));
};
