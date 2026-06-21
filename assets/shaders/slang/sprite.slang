// sprite.slang — sprite and text vertex + fragment shaders
#include "include/uniforms.slang"

struct VSInput
{
    float4 vertex : POSITION; // xy=pos, zw=texcoord
}

struct Varyings
{
    float4 pos      : SV_Position;
    float2 texCoord : TEXCOORD0;
}

[shader("vertex")]
Varyings vertMain(VSInput input)
{
    Varyings output;
    output.texCoord = input.vertex.zw;
    output.pos      = mul(spriteParams.projection, float4(input.vertex.xy, 0.0, 1.0));
    return output;
}

[[vk::binding(0)]] Sampler2D<float4> image;

[shader("fragment")]
float4 fragMain(Varyings input) : SV_Target
{
    float4 color = image.Sample(input.texCoord);
    if (color.a == 0.0)
    {
        discard;
    }
    color.a *= spriteParams.alpha;
    return color;
}

struct TextFSOutput
{
    [[vk::location(0)]] [[vk::index(0)]] float4 fragColor    : SV_Target0;
    [[vk::location(0)]] [[vk::index(1)]] float4 fragCoverage : SV_Target1;
}

[shader("fragment")]
TextFSOutput textFragMain(Varyings input)
{
    TextFSOutput output;
    output.fragColor    = float4(spriteParams.textColor, 1.0);
    output.fragCoverage = float4(image.Sample(input.texCoord).rgb, 1.0);
    return output;
}
