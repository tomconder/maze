// bloom.slang — brightness extraction, Dual Kawase bloom pipeline, composite

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
    float2 uv = input.texCoords;
    uint   w, h, levels;
    image.GetDimensions(0, w, h, levels);
    float2 o = float2(0.5 / float(w), 0.5 / float(h)) * offset;

    float3 color = image.Sample(uv).rgb * 4.0;
    color += image.Sample(uv + float2(-o.x, -o.y)).rgb;
    color += image.Sample(uv + float2( o.x, -o.y)).rgb;
    color += image.Sample(uv + float2(-o.x,  o.y)).rgb;
    color += image.Sample(uv + float2( o.x,  o.y)).rgb;
    return float4(color / 8.0, 1.0);
}

[shader("fragment")]
float4 upFragMain(FSInput input) : SV_Target
{
    float2 uv = input.texCoords;
    uint   w, h, levels;
    image.GetDimensions(0, w, h, levels);
    float2 o = float2(0.5 / float(w), 0.5 / float(h)) * offset;

    float3 color = float3(0.0);
    color += image.Sample(uv + float2(-o.x * 2.0,       0.0)).rgb;
    color += image.Sample(uv + float2( o.x * 2.0,       0.0)).rgb;
    color += image.Sample(uv + float2(0.0,        -o.y * 2.0)).rgb;
    color += image.Sample(uv + float2(0.0,         o.y * 2.0)).rgb;
    color += image.Sample(uv + float2(-o.x,  o.y)).rgb * 2.0;
    color += image.Sample(uv + float2( o.x,  o.y)).rgb * 2.0;
    color += image.Sample(uv + float2(-o.x, -o.y)).rgb * 2.0;
    color += image.Sample(uv + float2( o.x, -o.y)).rgb * 2.0;
    return float4(color / 12.0, 1.0);
}

[shader("fragment")]
float4 compositeFragMain(FSInput input) : SV_Target
{
    float3 color = image.Sample(input.texCoords).rgb;
    float3 bloom = bloomTex.Sample(input.texCoords).rgb;
    return float4(color + bloom * intensity, 1.0);
}
