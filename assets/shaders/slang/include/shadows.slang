#pragma once

// Must match ShadowMap::maxCascades in shadowmap.hpp.
static const int maxCascades = 4;

// Fraction of a cascade's width, at its edge, over which it blends into the
// next one.
static const float cascadeBlendBand = 0.1;

// EVSM lookup in one cascade. Returns shadow factor in [0,1] where 1 = fully
// shadowed.
float shadowEVSM(
    Sampler2DArray<float2> shadowMap,
    float4                 fragPosLightSpace,
    int                    layer,
    float                  evsmBleedThreshold) {
    float3 proj = fragPosLightSpace.xyz / fragPosLightSpace.w;
    proj        = proj * 0.5 + 0.5;
    // Outside the shadow frustum (xy or z) there is no occlusion information;
    // sampling clamped edge texels would falsely shadow everything beyond it.
    if (any(proj < 0.0) || any(proj > 1.0)) {
        return 0.0;
    }

    float  currentDepth = proj.z;
    float2 moments      = shadowMap.Sample(float3(proj.xy, float(layer)));

    float p        = float(currentDepth <= moments.x);
    float variance = max(moments.y - moments.x * moments.x, 0.00002);
    float d        = currentDepth - moments.x;
    float pMax     = variance / (variance + d * d);
    pMax           = clamp((pMax - evsmBleedThreshold) / (1.0 - evsmBleedThreshold), 0.0, 1.0);
    return 1.0 - max(p, pMax);
}

// Shadow factor from the finest cascade that holds the point, blended into
// the next one near its edge. The last cascade covers the whole scene, so it
// is the fallback. Picking by light-space position, not by camera depth, keeps
// it right for the mirrored view and the reflection probe, whose cameras are
// not the one the cascades follow.
float shadowCascaded(
    Sampler2DArray<float2> shadowMap,
    float4x4               lightSpace[maxCascades],
    int                    cascadeCount,
    float3                 worldPos,
    float                  evsmBleedThreshold) {
    float4 p = float4(worldPos, 1.0);
    for (int i = 0; i < cascadeCount - 1; i++) {
        float4 lp   = mul(lightSpace[i], p);
        float3 uvz  = lp.xyz / lp.w * 0.5 + 0.5;
        float2 edge = min(uvz.xy, 1.0 - uvz.xy);
        float  e    = min(edge.x, edge.y);
        if (e > 0.0 && uvz.z >= 0.0 && uvz.z <= 1.0) {
            float s = shadowEVSM(shadowMap, lp, i, evsmBleedThreshold);
            if (e >= cascadeBlendBand) {
                return s;
            }
            float sNext = shadowEVSM(shadowMap, mul(lightSpace[i + 1], p), i + 1, evsmBleedThreshold);
            return lerp(sNext, s, e / cascadeBlendBand);
        }
    }
    int last = max(cascadeCount - 1, 0);
    return shadowEVSM(shadowMap, mul(lightSpace[last], p), last, evsmBleedThreshold);
}
