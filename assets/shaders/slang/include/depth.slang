#pragma once

uniform float4x4 invProjection;

float3 viewPositionFromDepth(float2 uv, float depth) {
    float4 clip = float4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    float4 view = mul(invProjection, clip);
    return view.xyz / view.w;
}
