// assets/shaders/slang/include/clustered.slang
#pragma once

// Grid dimensions — must match kTilesX/Y/Z in clusteredlights.hpp and the GLSL compute shader.
static const int TILES_X = 8;
static const int TILES_Y = 4;
static const int TILES_Z = 8;

// Max lights per cluster — must match kMaxLightsPerCluster in clusteredlights.hpp.
static const int MAX_LIGHTS_PER_CLUSTER = 128;

// Conservative culling radii (world units) per attenuation index.
static const float attenuationRadius[11] = {
    7.0,  13.0,  20.0,  32.0,  50.0,  65.0,
    100.0, 160.0, 200.0, 325.0, 600.0
};

// Linearise gl_FragCoord.z (window-space depth [0,1]) to positive view-space depth.
float linearDepth(float windowZ, float near, float far)
{
    return 2.0 * near * far / (far + near - (2.0 * windowZ - 1.0) * (far - near));
}

// Returns the 1-D cluster index for the given fragment.
// windowZ: SV_Position.z (window-space depth [0,1]).
// Z subdivision formula must match buildClusterAABBs() in clusteredlights.cpp.
int clusterIndex(float2 fragCoord, float windowZ, float2 screenSize, float near, float far)
{
    int   x = clamp(int(fragCoord.x / screenSize.x * TILES_X), 0, TILES_X - 1);
    int   y = clamp(int(fragCoord.y / screenSize.y * TILES_Y), 0, TILES_Y - 1);
    float d = linearDepth(windowZ, near, far);
    int   z = clamp(int(log(d / near) / log(far / near) * float(TILES_Z)), 0, TILES_Z - 1);
    return x + y * TILES_X + z * TILES_X * TILES_Y;
}
