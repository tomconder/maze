// blur.slang — Dual Kawase blur for EVSM shadow maps (RG channels)

[[vk::binding(0)]] Sampler2D<float2> image;
uniform float offset;

struct FSInput { float2 texCoords : TEXCOORD0; }

[shader("fragment")]
float4 downFragMain(FSInput input) : SV_Target
{
    float2 uv = input.texCoords;
    uint   w, h, levels;
    image.GetDimensions(0, w, h, levels);
    float2 o = float2(0.5 / float(w), 0.5 / float(h)) * offset;

    float2 color = image.Sample(uv).rg * 4.0;
    color += image.Sample(uv + float2(-o.x, -o.y)).rg;
    color += image.Sample(uv + float2( o.x, -o.y)).rg;
    color += image.Sample(uv + float2(-o.x,  o.y)).rg;
    color += image.Sample(uv + float2( o.x,  o.y)).rg;
    return float4(color / 8.0, 0.0, 1.0);
}

[shader("fragment")]
float4 upFragMain(FSInput input) : SV_Target
{
    float2 uv = input.texCoords;
    uint   w, h, levels;
    image.GetDimensions(0, w, h, levels);
    float2 o = float2(0.5 / float(w), 0.5 / float(h)) * offset;

    float2 color = float2(0.0);
    color += image.Sample(uv + float2(-o.x * 2.0,       0.0)).rg;
    color += image.Sample(uv + float2( o.x * 2.0,       0.0)).rg;
    color += image.Sample(uv + float2(0.0,        -o.y * 2.0)).rg;
    color += image.Sample(uv + float2(0.0,         o.y * 2.0)).rg;
    color += image.Sample(uv + float2(-o.x,  o.y)).rg * 2.0;
    color += image.Sample(uv + float2( o.x,  o.y)).rg * 2.0;
    color += image.Sample(uv + float2(-o.x, -o.y)).rg * 2.0;
    color += image.Sample(uv + float2( o.x, -o.y)).rg * 2.0;
    return float4(color / 12.0, 0.0, 1.0);
}
