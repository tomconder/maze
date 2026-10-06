// shadowmap.slang: shadow depth vertex + fragment entry points
#include "include/shadows.slang"

uniform float4x4 lightSpaceMatrix;
uniform float4x4 model;
// glTF MASK, the same test as pbr.slang. The albedo texture is on unit 0.
uniform float                        alphaCutoff;     // 0 = opaque, no test
uniform float                        baseColorAlpha;  // baseColorFactor.a
uniform bool                         hasNoTexture;
uniform float4                       albedoUVTransform;  // xy = offset, zw = scale
[[vk::binding(0)]] Sampler2D<float4> albedoMap;

// texCoord keeps pbr.slang's attribute location (1) for the mesh VAO.
struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput {
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD0;
};

// clang-format off
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    // clang-format on
    VSOutput output;
    output.pos      = mul(lightSpaceMatrix, mul(model, float4(input.position, 1.0)));
    output.texCoord = input.texCoord;
    return output;
};

// Plain depth: written implicitly via SV_Position.z; no explicit output needed.
// clang-format off
[shader("fragment")]
void fragMain() {
    // clang-format on
};

// EVSM: store the first two moments of the exponentially warped depth, one
// pair for each warp sign (see shadows.slang).
// clang-format off
[shader("fragment")]
float4 fragMainEVSM(VSOutput input) : SV_Target {
    // clang-format on
    if (alphaCutoff > 0.0) {
        float2 uv    = input.texCoord * albedoUVTransform.zw + albedoUVTransform.xy;
        float  alpha = (hasNoTexture ? 1.0 : albedoMap.Sample(uv).a) * baseColorAlpha;
        if (alpha < alphaCutoff) {
            discard;
        }
    }
    float depth = input.pos.z;
    float pos   = exp(evsmExponent * depth);
    float neg   = -exp(-evsmExponent * depth);
    return float4(pos, pos * pos, neg, neg * neg);
};
