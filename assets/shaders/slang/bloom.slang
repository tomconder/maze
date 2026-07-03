// bloom.slang — brightness extraction, Dual Kawase bloom pipeline, composite
#include "include/kawase.slang"
#include "include/dither.slang"

[[vk::binding(0)]] Sampler2D<float4> image;
[[vk::binding(1)]] Sampler2D<float4> bloomTex;
uniform float threshold;
uniform float offset;
uniform float intensity;
uniform float accumulate;

struct FSInput { float2 texCoords : TEXCOORD0; }

[shader("fragment")]
float4 extractFragMain(FSInput input) : SV_Target
{
    float3 color      = image.Sample(input.texCoords).rgb;
    float  brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    // Soft knee (Karis) — a hard threshold leaves energy cliffs that survive
    // the blur chain as visible mip-texel edges.
    float knee = threshold * 0.5;
    float soft = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    soft       = soft * soft / (4.0 * knee + 1e-4);
    float contribution = max(soft, brightness - threshold);
    return float4(color * (contribution / max(brightness, 1e-4)), 1.0);
}

[shader("fragment")]
float4 downFragMain(FSInput input) : SV_Target
{
    return float4(kawaseDown(image, input.texCoords, offset).rgb, 1.0);
}

// Progressive upsample: blend the blurred coarse level with this level's
// downsample so no single coarse mip dominates (its texel structure would
// show as straight-edged rectangles). accumulate=0 at the deepest level.
// SCATTER weights the coarse chain: higher = wider, softer glow.
static const float SCATTER = 0.7;

[shader("fragment")]
float4 upFragMain(FSInput input) : SV_Target
{
    float3 blurred = kawaseUp(image, input.texCoords, offset).rgb;
    float3 down    = bloomTex.Sample(input.texCoords).rgb;
    return float4(lerp(blurred, lerp(down, blurred, SCATTER), accumulate), 1.0);
}

[shader("fragment")]
float4 compositeFragMain(FSInput input, float4 fragCoord : SV_Position) : SV_Target
{
    float3 color = image.Sample(input.texCoords).rgb;
    float3 bloom = bloomTex.Sample(input.texCoords).rgb;
    return float4(dither8(color + bloom * intensity, fragCoord.xy), 1.0);
}
