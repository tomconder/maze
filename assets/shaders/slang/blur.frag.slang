// blur.frag.slang — Dual Kawase downsample for EVSM shadow map (RG)

Texture2D<float2> image         : register(t0);
SamplerState      imageSampler  : register(s0);
uniform float     offset;

struct FSInput
{
    float2 texCoords : TEXCOORD0;
};

[shader("fragment")]
float4 fragMain(FSInput input) : SV_Target
{
    float2 uv = input.texCoords;
    uint   w, h, levels;
    image.GetDimensions(0, w, h, levels);
    float2 halfpixel = float2(0.5 / float(w), 0.5 / float(h));
    float2 o = halfpixel * offset;

    float2 color = image.Sample(imageSampler, uv).rg * 4.0;
    color += image.Sample(imageSampler, uv + float2(-o.x, -o.y)).rg;
    color += image.Sample(imageSampler, uv + float2( o.x, -o.y)).rg;
    color += image.Sample(imageSampler, uv + float2(-o.x,  o.y)).rg;
    color += image.Sample(imageSampler, uv + float2( o.x,  o.y)).rg;

    return float4(color / 8.0, 0.0, 1.0);
}
