// sprite.frag.slang
Texture2D<float4> image       : register(t0);
SamplerState      imageSampler: register(s0);
uniform float     alpha;

struct FSInput { float2 texCoord : TEXCOORD0; }

[shader("fragment")]
float4 fragMain(FSInput input) : SV_Target
{
    float4 color = image.Sample(imageSampler, input.texCoord);
    if (color.a == 0.0)
        discard;
    color.a *= alpha;
    return color;
}
