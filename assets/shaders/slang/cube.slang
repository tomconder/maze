// cube.slang: debug light-position cube vertex + fragment shader
uniform float4x4 mvp;
uniform float3   lightColor;

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
    output.pos   = mul(mvp, float4(input.position, 1.0));
    output.color = lightColor * emissiveStrength;
    return output;
};

// clang-format off
[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    // clang-format on
    return float4(input.color, 1.0);
};
