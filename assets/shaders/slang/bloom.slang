// bloom.slang: brightness extraction + Dual Kawase blur chain, as compute
// passes. A compute pass needs no framebuffer, so the chain does not pay for a
// render-target switch per level. The composite lives in tonemap.slang, which
// adds bloom in linear light.
#include "include/kawase.slang"

[[vk::binding(0)]] Sampler2D<float4> image;
// The matching down-level, accumulated by the upsample pass.
[[vk::binding(1)]] Sampler2D<float4>                       bloomTex;
[format("rgba16f")] [[vk::binding(2)]] RWTexture2D<float4> dst;
uniform float2                                             targetSize;  // destination size in pixels
uniform float                                              threshold;
uniform float                                              offset;
uniform float                                              accumulate;

// Centre of the pixel in the destination, the same uv a full-screen quad
// gives the fragment shader. False past the edge of the last group. The size
// is a uniform: slangc turns imageSize() into an extension this driver lacks.
bool pixelUv(uint3 tid, out float2 uv) {
    uv = (float2(tid.xy) + 0.5) / targetSize;
    return float(tid.x) < targetSize.x && float(tid.y) < targetSize.y;
}

// clang-format off
[shader("compute")] [numthreads(8, 8, 1)]
void extractMain(uint3 tid : SV_DispatchThreadID) {
    // clang-format on
    float2 uv;
    if (!pixelUv(tid, uv)) {
        return;
    }
    float3 color      = image.SampleLevel(uv, 0.0).rgb;
    float  brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    // Soft knee (Karis): a hard threshold leaves energy cliffs that survive
    // the blur chain as visible mip-texel edges.
    float knee         = threshold * 0.5;
    float soft         = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    soft               = soft * soft / (4.0 * knee + 1e-4);
    float contribution = max(soft, brightness - threshold);
    dst[tid.xy]        = float4(color * (contribution / max(brightness, 1e-4)), 1.0);
};

// clang-format off
[shader("compute")] [numthreads(8, 8, 1)]
void downMain(uint3 tid : SV_DispatchThreadID) {
    // clang-format on
    float2 uv;
    if (!pixelUv(tid, uv)) {
        return;
    }
    dst[tid.xy] = float4(kawaseDown(image, uv, offset).rgb, 1.0);
};

// Progressive upsample: blend the blurred coarse level with this level's
// downsample so no single coarse mip dominates (its texel structure would
// show as straight-edged rectangles). accumulate=0 at the deepest level.
// SCATTER weights the coarse chain: higher = wider, softer glow.
static const float SCATTER = 0.7;

// clang-format off
[shader("compute")] [numthreads(8, 8, 1)]
void upMain(uint3 tid : SV_DispatchThreadID) {
    // clang-format on
    float2 uv;
    if (!pixelUv(tid, uv)) {
        return;
    }
    float3 blurred = kawaseUp(image, uv, offset).rgb;
    float3 down    = bloomTex.SampleLevel(uv, 0.0).rgb;
    dst[tid.xy]    = float4(lerp(blurred, lerp(down, blurred, SCATTER), accumulate), 1.0);
};
