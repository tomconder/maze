// bloom_composite.frag.slang

[[vk::binding(0)]] Sampler2D<float4> scene;
[[vk::binding(1)]] Sampler2D<float4> bloomTex;
uniform float     intensity;

struct FSInput
{
    float2 texCoords : TEXCOORD0;
};

[shader("fragment")]
float4 fragMain(FSInput input) : SV_Target
{
    float3 color = scene.Sample(input.texCoords).rgb;
    float3 bloom = bloomTex.Sample(input.texCoords).rgb;
    return float4(color + bloom * intensity, 1.0);
}
