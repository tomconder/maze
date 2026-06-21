// quad.frag.slang — SDF rounded rect with optional border
uniform float4 corners;
uniform float  cornerRadius;
uniform float4 color;
uniform float  borderWidth;
uniform float4 borderColor;

struct FSInput
{
    float4 pos          : SV_Position;
    float2 localPosition: TEXCOORD0;
}

[shader("fragment")]
float4 fragMain(FSInput input) : SV_Target
{
    float2 localPos  = (input.localPosition - corners.xy) / (corners.zw - corners.xy);
    float2 quadSize  = corners.zw - corners.xy;
    float2 pos       = localPos * quadSize;
    float2 halfSize  = quadSize * 0.5;

    float2 d    = abs(pos - halfSize) - (halfSize - cornerRadius);
    float  dist = length(max(d, float2(0.0, 0.0))) - cornerRadius;
    float  alpha = smoothstep(1.0, 0.0, dist);

    if (borderWidth > 0.0)
    {
        float2 innerD    = abs(pos - halfSize) - (halfSize - cornerRadius - borderWidth);
        float  innerDist = length(max(innerD, float2(0.0, 0.0))) - cornerRadius;
        float  borderMask = smoothstep(1.0, 0.0, dist) - smoothstep(1.0, 0.0, innerDist);
        float  fillMask   = smoothstep(1.0, 0.0, innerDist);
        float4 finalColor = borderColor * borderMask + color * fillMask;
        return float4(finalColor.rgb, finalColor.a * alpha);
    }
    return float4(color.rgb, color.a * alpha);
}
