// tonemap.slang: resolves the linear HDR scene to a display image.
// Composites bloom in linear light, then tone maps and gamma encodes. This is
// the only place the pipeline leaves linear space — the scene pass and the
// bloom chain both run on unbounded radiance, so the bloom threshold is a real
// radiance value rather than a point on an already-compressed curve.
#include "include/dither.slang"

struct FSInput {
    float2 texCoords : TEXCOORD0;
};

[[vk::binding(0)]] Sampler2D<float4> sceneTex;
[[vk::binding(1)]] Sampler2D<float4> bloomTex;

uniform float bloomIntensity;
// True only when this pass writes the final 8-bit target. Anti-aliasing
// runs after it and dithers on its own write, and dithering into an
// intermediate float target would just seed noise for TAA to accumulate.
uniform bool ditherOutput;

// clang-format off
[shader("fragment")]
float4 fragMain(FSInput input, float4 fragCoord : SV_Position) : SV_Target {
    // clang-format on
    float3 color = sceneTex.Sample(input.texCoords).rgb;
    if (bloomIntensity > 0.0) {
        color += bloomTex.Sample(input.texCoords).rgb * bloomIntensity;
    }

    // Reinhard
    color = color / (color + float3(1.0));

    color = pow(color, float3(1.0 / 2.2));

    if (ditherOutput) {
        color = dither8(color, fragCoord.xy);
    }
    return float4(color, 1.0);
};
