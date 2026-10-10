// cube.slang: debug light-position cube vertex + fragment shader. One instanced
// draw covers every light: instance i reads its cube from cubeInstances.
#include "include/instance.slang"

uniform float4x4 mvp;
uniform float    cubeScale;

// Per cube, three float4: position, previous position (the depth prepass uses
// it) and colour, each in xyz. Must match Cube::Instance in cube.hpp.
[[vk::binding(8)]] StructuredBuffer<float4> cubeInstances;

// The cubes mark light positions, so they read as emitters. Radiance 1.0 is
// mid-grey once tone mapped (1/(1+1) = 0.5); this keeps them near white and
// above the bloom threshold, which is now a linear radiance value.
static const float emissiveStrength = 8.0;

struct VSInput {
    float3 position : POSITION;
};
struct VSOutput {
    float4 pos : SV_Position;
    float3 color : TEXCOORD0;
};

// clang-format off
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    // clang-format on
    VSOutput output;
    uint     instance = instanceIndex();
    float3   center   = cubeInstances[instance * 3].xyz;
    output.pos        = mul(mvp, float4(input.position * cubeScale + center, 1.0));
    output.color      = cubeInstances[instance * 3 + 2].xyz * emissiveStrength;
    return output;
};

// clang-format off
[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    // clang-format on
    return float4(input.color, 1.0);
};
