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
// Must match the order of ToneMapper in scenetarget.hpp.
uniform int toneMapper;

static const int toneReinhard = 0;
static const int toneFilmic   = 1;
static const int toneAces     = 2;
static const int toneAgx      = 3;

// Uncharted 2 curve (Hable). Exposure bias 2 and white point 11.2 as in the
// original talk.
float3 hable(float3 x) {
    const float a = 0.15, b = 0.50, c = 0.10, d = 0.20, e = 0.02, f = 0.30;
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

float3 filmic(float3 color) {
    return hable(color * 2.0) / hable(float3(11.2));
}

// Narkowicz fit of the ACES RRT and ODT.
float3 aces(float3 x) {
    return saturate((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14));
}

// AgX with the polynomial contrast fit (minimal AgX, Wrensch). Returns linear
// display light, so the shared gamma encode below applies as for the others.
float3 agx(float3 color) {
    const float3x3 inset = float3x3(
        0.842479062253094, 0.0784335999999992, 0.0792237451477643,
        0.0423282422610123, 0.878468636469772, 0.0791661274605434,
        0.0423756549057051, 0.0784336, 0.879142973793104);
    const float3x3 outset = float3x3(
        1.19687900512017, -0.0980208811401368, -0.0990297440797205,
        -0.0528968517574562, 1.15190312990417, -0.0989611768448433,
        -0.0529716355144438, -0.0980434501171241, 1.15107367264116);
    const float minEv = -12.47393;
    const float maxEv = 4.026069;

    float3 v = mul(inset, color);
    v        = clamp(log2(max(v, float3(1e-10))), minEv, maxEv);
    v        = (v - minEv) / (maxEv - minEv);

    float3 v2 = v * v;
    float3 v4 = v2 * v2;
    v         = 15.5 * v4 * v2 - 40.14 * v4 * v + 31.96 * v4 - 6.868 * v2 * v +
                0.4298 * v2 + 0.1191 * v - 0.00232;

    v = mul(outset, v);
    return pow(max(v, float3(0.0)), float3(2.2));
}

// clang-format off
[shader("fragment")]
float4 fragMain(FSInput input, float4 fragCoord : SV_Position) : SV_Target {
    // clang-format on
    float3 color = sceneTex.Sample(input.texCoords).rgb;
    if (bloomIntensity > 0.0) {
        color += bloomTex.Sample(input.texCoords).rgb * bloomIntensity;
    }

    if (toneMapper == toneFilmic) {
        color = filmic(color);
    } else if (toneMapper == toneAces) {
        color = aces(color);
    } else if (toneMapper == toneAgx) {
        color = agx(color);
    } else {
        color = color / (color + float3(1.0));
    }

    color = pow(color, float3(1.0 / 2.2));

    if (ditherOutput) {
        color = dither8(color, fragCoord.xy);
    }
    return float4(color, 1.0);
};
