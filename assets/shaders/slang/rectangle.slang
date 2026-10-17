// rectangle.slang: UI rounded-rect vertex + SDF fragment shader
#include "include/ui_uniforms.slang"

struct VSInput {
    float2 position : POSITION;
};

struct VSOutput {
    float4 pos : SV_Position;
    float2 localPosition : TEXCOORD0;
};

// clang-format off
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    // clang-format on
    VSOutput output;
    output.pos           = mul(quadParams.projection, float4(input.position, 0.0, 1.0));
    output.localPosition = input.position;
    return output;
};

// Signed distance to a rounded box of half extents h; negative inside.
float sdRoundBox(float2 p, float2 h, float r) {
    float2 d = abs(p) - (h - r);
    return length(max(d, float2(0.0, 0.0))) + min(max(d.x, d.y), 0.0) - r;
}

// clang-format off
[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    // clang-format on
    float2 halfSize = (quadParams.corners.zw - quadParams.corners.xy) * 0.5;
    float2 p        = input.localPosition - quadParams.corners.xy - halfSize;

    // Coverage is centred on the edge, in pixels.
    float radius = min(quadParams.cornerRadius, min(halfSize.x, halfSize.y));
    float alpha  = saturate(0.5 - sdRoundBox(p, halfSize, radius));

    if (quadParams.borderWidth > 0.0) {
        // The inner edge shares the corner centre (radius shrinks by the border width), so the
        // border keeps a constant thickness around the corners.
        float  bw         = quadParams.borderWidth;
        float  innerAlpha = saturate(0.5 - sdRoundBox(p, halfSize - bw, max(radius - bw, 0.0)));
        float  borderMask = alpha - innerAlpha;
        float4 finalColor = quadParams.borderColor * borderMask + quadParams.color * innerAlpha;
        return float4(finalColor.rgb, finalColor.a * alpha);
    }
    return float4(quadParams.color.rgb, quadParams.color.a * alpha);
};
