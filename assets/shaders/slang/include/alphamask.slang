#pragma once

// glTF MASK, the same test as pbr.slang, for the passes that run before it
// (depth prepass, shadow map). Only the masked fragment entry points call it;
// each is its own program because a discard turns off early-Z for every draw of
// the program. The albedo texture is on unit 0.
uniform float                        alphaCutoff;     // 0 = opaque, no test
uniform float                        baseColorAlpha;  // baseColorFactor.a
uniform bool                         hasNoTexture;
uniform float4                       albedoUVTransform;  // xy = offset, zw = scale
[[vk::binding(0)]] Sampler2D<float4> albedoMap;

bool maskedOut(float2 texCoord) {
    float2 uv    = texCoord * albedoUVTransform.zw + albedoUVTransform.xy;
    float  alpha = (hasNoTexture ? 1.0 : albedoMap.Sample(uv).a) * baseColorAlpha;
    return alpha < alphaCutoff;
}
