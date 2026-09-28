// planar.slang: draws a planar mirror from its reflection image. The image
// was rendered from the camera reflected in the mirror plane, so each mirror
// pixel reads the same screen position in it. The mesh's side faces are not
// part of the mirror plane, so a fragment whose world normal disagrees with
// the plane normal is discarded. The top face needs flat, outward normals:
// smoothed or flipped normals discard the whole mirror without a warning.
struct PlanarParams {
    float4x4 mvp;
    float4x4 normalMatrix;
    float2   screenSize;
    float    strength;
    float3   planeNormal;
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

struct VSOutput {
    float4 pos : SV_Position;
    float3 worldNormal : TEXCOORD0;
};

// clang-format off
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    // clang-format on
    VSOutput output;
    output.pos         = mul(params.mvp, float4(input.position, 1.0));
    output.worldNormal = mul((float3x3)params.normalMatrix, input.normal);
    return output;
};

// clang-format off
[shader("fragment")]
float4 fragMain(VSOutput input, float4 fragCoord : SV_Position) : SV_Target {
    // clang-format on
    if (dot(normalize(input.worldNormal), params.planeNormal) < 0.99) {
        discard;
    }
    float3 color = reflection.Sample(fragCoord.xy / params.screenSize).rgb;
    return float4(color, params.strength);
}
