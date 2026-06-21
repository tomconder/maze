// blur.frag.slang — Dual Kawase downsample for EVSM shadow map (RG)

[[vk::binding(0)]] Sampler2D<float2> image;
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

    float2 color = image.Sample(uv).rg * 4.0;
    color += image.Sample(uv + float2(-o.x, -o.y)).rg;
    color += image.Sample(uv + float2( o.x, -o.y)).rg;
    color += image.Sample(uv + float2(-o.x,  o.y)).rg;
    color += image.Sample(uv + float2( o.x,  o.y)).rg;

    return float4(color / 8.0, 0.0, 1.0);
}
