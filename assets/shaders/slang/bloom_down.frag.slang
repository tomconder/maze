// bloom_down.frag.slang — Dual Kawase downsample for bloom (RGB)

Texture2D<float4> image        : register(t0);
SamplerState      imageSampler : register(s0);
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

    float3 color = image.Sample(imageSampler, uv).rgb * 4.0;
    color += image.Sample(imageSampler, uv + float2(-o.x, -o.y)).rgb;
    color += image.Sample(imageSampler, uv + float2( o.x, -o.y)).rgb;
    color += image.Sample(imageSampler, uv + float2(-o.x,  o.y)).rgb;
    color += image.Sample(imageSampler, uv + float2( o.x,  o.y)).rgb;

    return float4(color / 8.0, 1.0);
}
