// bloom.slang — brightness extraction, Dual Kawase bloom pipeline, composite
#include "include/kawase.slang"

[[vk::binding(0)]] Sampler2D<float4> image;
[[vk::binding(1)]] Sampler2D<float4> bloomTex;
uniform float threshold;
uniform float offset;
uniform float intensity;

struct FSInput { float2 texCoords : TEXCOORD0; }

[shader("fragment")]
float4 extractFragMain(FSInput input) : SV_Target
{
    float3 color      = image.Sample(input.texCoords).rgb;
    float  brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    return brightness > threshold ? float4(color, 1.0) : float4(0.0, 0.0, 0.0, 1.0);
}

[shader("fragment")]
float4 downFragMain(FSInput input) : SV_Target
{
    return float4(kawaseDown(image, input.texCoords, offset).rgb, 1.0);
}

[shader("fragment")]
float4 upFragMain(FSInput input) : SV_Target
{
    return float4(kawaseUp(image, input.texCoords, offset).rgb, 1.0);
}

[shader("fragment")]
float4 compositeFragMain(FSInput input) : SV_Target
{
    float3 color = image.Sample(input.texCoords).rgb;
    float3 bloom = bloomTex.Sample(input.texCoords).rgb;
    return float4(color + bloom * intensity, 1.0);
}
