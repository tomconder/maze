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

// clang-format off
[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    // clang-format on
    float2 localPos = (input.localPosition - quadParams.corners.xy) / (quadParams.corners.zw - quadParams.corners.xy);
    float2 quadSize = quadParams.corners.zw - quadParams.corners.xy;
    float2 pos      = localPos * quadSize;
    float2 halfSize = quadSize * 0.5;

    float2 d     = abs(pos - halfSize) - (halfSize - quadParams.cornerRadius);
    float  dist  = length(max(d, float2(0.0, 0.0))) - quadParams.cornerRadius;
    float  alpha = smoothstep(1.0, 0.0, dist);

    if (quadParams.borderWidth > 0.0) {
        float2 innerD     = abs(pos - halfSize) - (halfSize - quadParams.cornerRadius - quadParams.borderWidth);
        float  innerDist  = length(max(innerD, float2(0.0, 0.0))) - quadParams.cornerRadius;
        float  borderMask = smoothstep(1.0, 0.0, dist) - smoothstep(1.0, 0.0, innerDist);
        float  fillMask   = smoothstep(1.0, 0.0, innerDist);
        float4 finalColor = quadParams.borderColor * borderMask + quadParams.color * fillMask;
        return float4(finalColor.rgb, finalColor.a * alpha);
    }
    return float4(quadParams.color.rgb, quadParams.color.a * alpha);
};
