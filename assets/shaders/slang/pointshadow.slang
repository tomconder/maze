// pointshadow.slang: distance from a point light to the nearest surface, one
// cube face at a time. The value is divided by the light's range, so 1 is the
// edge of its reach and the clear value reads as lit.
#include "include/alphamask.slang"

uniform float4x4 viewProj;
uniform float4x4 model;
uniform float3   lightPos;
uniform float    lightRange;

// texCoord keeps pbr.slang's attribute location (1) for the mesh VAO.
struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput {
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

// clang-format off
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    // clang-format on
    VSOutput output;
    float4   world  = mul(model, float4(input.position, 1.0));
    output.pos      = mul(viewProj, world);
    output.texCoord = input.texCoord;
    output.worldPos = world.xyz;
    return output;
};

// clang-format off
[shader("fragment")]
float fragMain(VSOutput input) : SV_Target {
    // clang-format on
    return length(input.worldPos - lightPos) / lightRange;
};

// clang-format off
[shader("fragment")]
float fragMainMasked(VSOutput input) : SV_Target {
    // clang-format on
    if (maskedOut(input.texCoord)) {
        discard;
    }
    return length(input.worldPos - lightPos) / lightRange;
};
