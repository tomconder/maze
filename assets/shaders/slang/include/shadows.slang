#pragma once

// Must match ShadowMap::maxCascades in shadowmap.hpp.
static const int maxCascades = 4;

// Must match ShadowMap::evsmExponent in shadowmap.hpp. exp(2 * c) must stay
// below the float32 maximum (about 3.4e38), so c is at most about 44.
static const float evsmExponent = 40.0;

// Smallest depth spread the variance may shrink to, in light-space depth.
static const float evsmMinSpread = 0.0005;

// Fraction of a cascade's width, at its edge, over which it blends into the
// next one.
static const float cascadeBlendBand = 0.1;

// Chebyshev upper bound of P(occluder depth >= t) from the moments of one
// warp. minVariance keeps a flat caster from reading as a zero-width spread.
float chebyshev(float2 moments, float t, float minVariance) {
    if (t <= moments.x) {
        return 1.0;
    }
    float variance = max(moments.y - moments.x * moments.x, minVariance);
    float d        = t - moments.x;
    return variance / (variance + d * d);
}

// EVSM lookup in one cascade. Returns shadow factor in [0,1] where 1 = fully
// shadowed. The bleed threshold trims the low end of the lit factor.
float shadowEVSM(
    Sampler2DArray<float4> shadowMap,
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

    float4 m   = shadowMap.Sample(float3(proj.xy, float(layer)));
    float  pos = exp(evsmExponent * proj.z);
    float  neg = -exp(-evsmExponent * proj.z);

    float posSpread = evsmMinSpread * evsmExponent * pos;
    float negSpread = evsmMinSpread * evsmExponent * -neg;
    float lit       = min(
        chebyshev(m.xy, pos, posSpread * posSpread),
        chebyshev(m.zw, neg, negSpread * negSpread));
    lit = clamp((lit - evsmBleedThreshold) / (1.0 - evsmBleedThreshold), 0.0, 1.0);
    return 1.0 - lit;
}

// Shadow factor from the finest cascade that holds the point, blended into
// the next one near its edge. The last cascade covers the whole scene, so it
// is the fallback. Picking by light-space position, not by camera depth, keeps
// it right for the mirrored view and the reflection probe, whose cameras are
// not the one the cascades follow.
float shadowCascaded(
    Sampler2DArray<float4> shadowMap,
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

// Must match PointShadowMap::maxLights in pointshadowmap.hpp.
static const int maxPointShadows = 8;

// Shadow bias for a point light, in metres: a fixed part, plus a part that
// grows with the distance, since a texel covers more of the world far from the
// light. The surface is also moved along its normal by the same fixed part.
static const float pointShadowBias      = 0.04;
static const float pointShadowSlopeBias = 0.01;

// Half-width of the filter, in the cube lookup direction. A texel is 2 / 512.
static const float pointShadowFilter = 0.006;

// Shadow factor of one point light, where 1 = fully shadowed. The cube holds
// distance / range, so the lookup is by direction alone. Four taps around the
// direction soften the edge.
float shadowPoint(
    SamplerCubeArray<float4> pointShadowMap,
    int                      slot,
    float3                   lightPos,
    float3                   worldPos,
    float3                   normal,
    float                    range) {
    float3 toFrag = worldPos + normal * pointShadowBias - lightPos;
    float  dist   = length(toFrag);
    if (dist >= range) {
        return 0.0;
    }
    float3 dir  = toFrag / dist;
    float  ref  = (dist - pointShadowBias - pointShadowSlopeBias * dist) / range;
    float3 up   = abs(dir.y) < 0.99 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 side = normalize(cross(dir, up));
    float3 high = cross(dir, side);

    float lit = 0.0;
    for (int i = 0; i < 4; i++) {
        float2 o      = float2((i & 1) == 0 ? -1.0 : 1.0, (i & 2) == 0 ? -1.0 : 1.0) * pointShadowFilter;
        float  stored = pointShadowMap.SampleLevel(float4(dir + side * o.x + high * o.y, float(slot)), 0.0).r;
        lit += ref <= stored ? 1.0 : 0.0;
    }
    return 1.0 - lit * 0.25;
}
