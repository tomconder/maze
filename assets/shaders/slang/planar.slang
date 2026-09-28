// planar.slang: draws a planar mirror from its reflection image. The image
// was rendered from the camera reflected in the mirror plane, so each mirror
// pixel reads the same screen position in it.
struct PlanarParams {
    float4x4 mvp;
    float2   screenSize;
    float    strength;
};
[[vk::binding(2)]] ConstantBuffer<PlanarParams> params;
[[vk::binding(11)]] Sampler2D<float4>           reflection;

// Same VSInput layout as pbr.slang so the mesh VAO locations match.
struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

// clang-format off
[shader("vertex")]
float4 vertMain(VSInput input) : SV_Position {
    // clang-format on
    return mul(params.mvp, float4(input.position, 1.0));
}

// clang-format off
[shader("fragment")]
float4 fragMain(float4 fragCoord : SV_Position) : SV_Target {
    // clang-format on
    float3 color = reflection.Sample(fragCoord.xy / params.screenSize).rgb;
    return float4(color, params.strength);
}
