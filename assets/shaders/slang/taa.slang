// taa.slang: temporal anti-aliasing resolve (velocity reprojection + 3x3
// neighborhood clamp) and the dithered present blit.
// Geometry covered by the depth prepass carries its own screen-space motion,
// so an object that moves reprojects correctly. Pixels the prepass never
// touched (background, light cubes) fall back to reconstructing the world
// position from depth and reprojecting it with the camera alone.
#include "include/dither.slang"

struct FSInput {
    float2 texCoords : TEXCOORD0;
};

[[vk::binding(0)]] Sampler2D<float4> currentTex;
[[vk::binding(1)]] Sampler2D<float4> historyTex;
[[vk::binding(2)]] Sampler2D<float4> depthTex;
[[vk::binding(4)]] Sampler2D<float4> velocityTex;

uniform float2 rcpFrame;
// 1.0 discards history entirely (first frame after a resize or a mode change)
uniform float    historyBlend;
uniform float4x4 invViewProj;   // inverse of this frame's jittered view-proj
uniform float4x4 prevViewProj;  // last frame's unjittered view-proj

// The input is already the resolved display image (bloom composited, tone
// mapped), so the clamp box and the history are in the same space.
float3 sceneColor(float2 uv) {
    return currentTex.Sample(uv).rgb;
}

// Catmull-Rom history filter (5-tap form of the 9-tap bicubic).
// prevUV almost never lands on a texel centre while the camera moves, so the
// history is resampled every frame. Bilinear iterated ~10 deep is a low-pass
// filter and the image goes soft in motion; Catmull-Rom's negative lobes keep
// the high frequencies. Its overshoot is bounded by the neighbourhood clamp
// applied to the result.
float3 sampleHistory(float2 uv) {
    float2 texSize   = 1.0 / rcpFrame;
    float2 samplePos = uv * texSize;
    float2 texPos1   = floor(samplePos - 0.5) + 0.5;
    float2 f         = samplePos - texPos1;

    float2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    float2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    float2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    float2 w3 = f * f * (-0.5 + 0.5 * f);

    // Fold the middle pair into one bilinear tap at their weighted centre.
    float2 w12      = w1 + w2;
    float2 offset12 = w2 / w12;

    float2 texPos0  = (texPos1 - 1.0) * rcpFrame;
    float2 texPos3  = (texPos1 + 2.0) * rcpFrame;
    float2 texPos12 = (texPos1 + offset12) * rcpFrame;

    float3 result = float3(0.0);
    result += historyTex.Sample(float2(texPos12.x, texPos0.y)).rgb * w12.x * w0.y;
    result += historyTex.Sample(float2(texPos0.x, texPos12.y)).rgb * w0.x * w12.y;
    result += historyTex.Sample(float2(texPos12.x, texPos12.y)).rgb * w12.x * w12.y;
    result += historyTex.Sample(float2(texPos3.x, texPos12.y)).rgb * w3.x * w12.y;
    result += historyTex.Sample(float2(texPos12.x, texPos3.y)).rgb * w12.x * w3.y;

    // Dropping the four corner taps loses a little weight (~1.6% at the worst
    // sub-texel offset), so renormalise rather than dim the history.
    float weight = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y +
                   w3.x * w12.y + w12.x * w3.y;
    return result / weight;
}

// clang-format off
[shader("fragment")]
float4 resolveFragMain(FSInput input) : SV_Target {
    // clang-format on
    float2 uv      = input.texCoords;
    float3 current = sceneColor(uv);

    // Depth is 1.0 exactly where the prepass drew nothing, so it doubles as
    // the velocity buffer's coverage mask — no sentinel value needed.
    float  depth = depthTex.Sample(uv).r;
    float2 prevUV;
    bool   valid;
    if (depth < 1.0) {
        prevUV = uv - velocityTex.Sample(uv).rg;
        valid  = true;
    } else {
        // Reconstruct at the far plane and reproject with the camera alone.
        float4 clipPos  = float4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
        float4 worldPos = mul(invViewProj, clipPos);
        worldPos /= worldPos.w;
        float4 prevClip = mul(prevViewProj, worldPos);
        prevUV          = (prevClip.xy / prevClip.w) * 0.5 + 0.5;
        valid           = prevClip.w > 0.0;
    }

    // Written as positive tests so a non-finite prevUV fails them and discards
    // the history: one NaN in a float history poisons it for good.
    valid = valid && all(prevUV >= 0.0) && all(prevUV <= 1.0);

    float blend = valid ? historyBlend : 1.0;

    if (blend >= 1.0) {
        return float4(current, 1.0);
    }

    // Clamping the history to the current 3x3 colour box is what keeps
    // reprojection error (disocclusion, geometry missing from the prepass)
    // from turning into a ghost trail.
    float3 boxMin = current;
    float3 boxMax = current;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            float3 neighbor =
                sceneColor(uv + float2(x, y) * rcpFrame);
            boxMin = min(boxMin, neighbor);
            boxMax = max(boxMax, neighbor);
        }
    }

    float3 history = clamp(sampleHistory(prevUV), boxMin, boxMax);
    return float4(lerp(history, current, blend), 1.0);
};

// clang-format off
[shader("fragment")]
float4 presentFragMain(FSInput input, float4 fragCoord : SV_Position) : SV_Target {
    // clang-format on
    // The slot just written by resolveFragMain, still on binding 1.
    float3 color = historyTex.Sample(input.texCoords).rgb;
    return float4(dither8(color, fragCoord.xy), 1.0);
};
