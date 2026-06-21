// bloom_composite.frag.slang

Texture2D<float4> scene        : register(t0);
SamplerState      sceneSampler : register(s0);
Texture2D<float4> bloomTex     : register(t1);
SamplerState      bloomSampler : register(s1);
uniform float     intensity;

struct FSInput
{
    float2 texCoords : TEXCOORD0;
};

[shader("fragment")]
float4 fragMain(FSInput input) : SV_Target
{
    float3 color = scene.Sample(sceneSampler,  input.texCoords).rgb;
    float3 bloom = bloomTex.Sample(bloomSampler, input.texCoords).rgb;
    return float4(color + bloom * intensity, 1.0);
}
