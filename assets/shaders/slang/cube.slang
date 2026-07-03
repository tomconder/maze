// cube.slang: debug light-position cube vertex + fragment shader
uniform float4x4 mvp;
uniform float3   lightColor;

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
    output.color = lightColor;
    return output;
};

// clang-format off
[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    // clang-format on
    return float4(input.color, 1.0);
};
