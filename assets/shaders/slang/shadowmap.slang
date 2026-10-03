// shadowmap.slang: shadow depth vertex + fragment entry points
#include "include/shadows.slang"

uniform float4x4 lightSpaceMatrix;
uniform float4x4 model;

struct VSInput {
    float3 position : POSITION;
};

struct VSOutput {
    float4 pos : SV_Position;
};

// clang-format off
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    // clang-format on
    VSOutput output;
    output.pos = mul(lightSpaceMatrix, mul(model, float4(input.position, 1.0)));
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
    float depth = input.pos.z;
    float pos   = exp(evsmExponent * depth);
    float neg   = -exp(-evsmExponent * depth);
    return float4(pos, pos * pos, neg, neg * neg);
};
