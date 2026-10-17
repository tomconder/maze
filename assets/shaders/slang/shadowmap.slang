// shadowmap.slang: shadow depth vertex + fragment entry points
#include "include/alphamask.slang"
#include "include/shadows.slang"

uniform float4x4 lightSpaceMatrix;
uniform float4x4 model;

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

// EVSM: store the first two moments of the exponentially warped depth, one
// pair for each warp sign (see shadows.slang).
float4 evsmMoments(VSOutput input) {
    float depth = input.pos.z;
    float pos   = exp(evsmExponent * depth);
    float neg   = -exp(-evsmExponent * depth);
    return float4(pos, pos * pos, neg, neg * neg);
}

// clang-format off
[shader("fragment")]
float4 fragMainEVSM(VSOutput input) : SV_Target {
    // clang-format on
    return evsmMoments(input);
};

// clang-format off
[shader("fragment")]
float4 fragMainEVSMMasked(VSOutput input) : SV_Target {
    // clang-format on
    if (maskedOut(input.texCoord)) {
        discard;
    }
    return evsmMoments(input);
};
