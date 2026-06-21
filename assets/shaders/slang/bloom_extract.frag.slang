// bloom_extract.frag.slang

[[vk::binding(0)]] Sampler2D<float4> scene;
uniform float     threshold;

struct FSInput
{
    float2 texCoords : TEXCOORD0;
};

[shader("fragment")]
float4 fragMain(FSInput input) : SV_Target
{
    float3 color      = scene.Sample(input.texCoords).rgb;
    float  brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    return brightness > threshold
        ? float4(color, 1.0)
        : float4(0.0, 0.0, 0.0, 1.0);
}
