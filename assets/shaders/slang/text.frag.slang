// text.frag.slang — dual-source blending for sub-pixel text rendering
#include "include/uniforms.slang"

[[vk::binding(0)]] Sampler2D<float4> text;

struct FSInput { float2 texCoord : TEXCOORD0; }

struct FSOutput
{
    [[vk::location(0)]] [[vk::index(0)]] float4 fragColor    : SV_Target0;
    [[vk::location(0)]] [[vk::index(1)]] float4 fragCoverage : SV_Target1;
}

[shader("fragment")]
FSOutput fragMain(FSInput input)
{
    FSOutput output;
    output.fragColor    = float4(spriteParams.textColor, 1.0);
    output.fragCoverage = float4(text.Sample(input.texCoord).rgb, 1.0);
    return output;
}
