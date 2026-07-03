// assets/shaders/slang/include/clustered.slang
#pragma once
#include "lighting.slang"

// Grid dimensions — must match kTilesX/Y/Z in clusteredlights.hpp and the GLSL compute shader.
static const int TILES_X = 16;
static const int TILES_Y = 9;
static const int TILES_Z = 24;

// Max lights per cluster — must match maxLightsPerCluster in clusteredlights.hpp.
// Sized >= the max scene light count so per-cluster truncation cannot occur.
static const int MAX_LIGHTS_PER_CLUSTER = 512;

// Culling radii live in lighting.slang (attenuationRadius) — shared with the
// radiance window so culled lights are exactly zero at the boundary.

// Returns the 1-D cluster index for the given fragment.
// viewZ: positive view-space depth — dot(worldPos - cameraPos, cameraForward).
// Note: decoding gl_FragCoord.z instead was tried and fails — it disagrees
// with the view-space slice AABBs on this pipeline (verified empirically).
// Z subdivision formula must match buildClusterAABBs() in clusteredlights.cpp.
int clusterIndex(float2 fragCoord, float viewZ, float2 screenSize, float near, float far)
{
    int   x = clamp(int(fragCoord.x / screenSize.x * TILES_X), 0, TILES_X - 1);
    int   y = clamp(int(fragCoord.y / screenSize.y * TILES_Y), 0, TILES_Y - 1);
    float d = max(viewZ, near);
    int   z = clamp(int(log(d / near) / log(far / near) * float(TILES_Z)), 0, TILES_Z - 1);
    return x + y * TILES_X + z * TILES_X * TILES_Y;
}
