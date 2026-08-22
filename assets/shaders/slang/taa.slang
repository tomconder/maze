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
[[vk::binding(3)]] Sampler2D<float4> bloomTex;
[[vk::binding(4)]] Sampler2D<float4> velocityTex;

uniform float2 rcpFrame;
uniform float  bloomIntensity;
// 1.0 discards history entirely (first frame after a resize or a mode change)
uniform float    historyBlend;
uniform float4x4 invViewProj;   // inverse of this frame's jittered view-proj
uniform float4x4 prevViewProj;  // last frame's unjittered view-proj

// Scene colour as it will be presented, so the clamp box and the history are
// in the same space.
float3 sceneColor(float2 uv) {
    float3 color = currentTex.Sample(uv).rgb;
    if (bloomIntensity > 0.0) {
        color += bloomTex.Sample(uv).rgb * bloomIntensity;
    }
    return color;
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

    float3 history = clamp(historyTex.Sample(prevUV).rgb, boxMin, boxMax);
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
