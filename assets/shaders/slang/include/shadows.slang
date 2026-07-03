#pragma once

// EVSM shadow lookup. Returns shadow factor in [0,1] where 1 = fully shadowed.
float shadowEVSM(
    Sampler2D<float2> shadowMap,
    float4            fragPosLightSpace,
    float             evsmBleedThreshold) {
    float3 proj = fragPosLightSpace.xyz / fragPosLightSpace.w;
    proj        = proj * 0.5 + 0.5;
    // Outside the shadow frustum (xy or z) there is no occlusion information;
    // sampling clamped edge texels would falsely shadow everything beyond it.
    if (any(proj < 0.0) || any(proj > 1.0)) {
        return 0.0;
    }

    float  currentDepth = proj.z;
    float2 moments      = shadowMap.Sample(proj.xy);

    float p        = float(currentDepth <= moments.x);
    float variance = max(moments.y - moments.x * moments.x, 0.00002);
    float d        = currentDepth - moments.x;
    float pMax     = variance / (variance + d * d);
    pMax           = clamp((pMax - evsmBleedThreshold) / (1.0 - evsmBleedThreshold), 0.0, 1.0);
    return 1.0 - max(p, pMax);
}
