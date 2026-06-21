// bloom_up.frag.slang — Dual Kawase upsample for bloom (RGB)

[[vk::binding(0)]] Sampler2D<float4> image;
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

    float3 color = float3(0.0);
    color += image.Sample(uv + float2(-o.x * 2.0, 0.0)).rgb;
    color += image.Sample(uv + float2( o.x * 2.0, 0.0)).rgb;
    color += image.Sample(uv + float2(0.0, -o.y * 2.0)).rgb;
    color += image.Sample(uv + float2(0.0,  o.y * 2.0)).rgb;
    color += image.Sample(uv + float2(-o.x,  o.y)).rgb * 2.0;
    color += image.Sample(uv + float2( o.x,  o.y)).rgb * 2.0;
    color += image.Sample(uv + float2(-o.x, -o.y)).rgb * 2.0;
    color += image.Sample(uv + float2( o.x, -o.y)).rgb * 2.0;

    return float4(color / 12.0, 1.0);
}
