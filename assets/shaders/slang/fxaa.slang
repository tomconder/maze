// fxaa.slang — fullscreen quad vertex + FXAA 3.11 (quality preset 12) + bloom overlay
// FXAA based on Timothy Lottes / NVIDIA, adapted to Slang.
#include "include/dither.slang"

struct VSInput
{
    float2 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 pos       : SV_Position;
    float2 texCoords : TEXCOORD0;
};

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    output.pos       = float4(input.position, 0.0, 1.0);
    output.texCoords = input.texCoord;
    return output;
}

[[vk::binding(0)]] Sampler2D<float4> screenTexture;
[[vk::binding(1)]] Sampler2D<float4> bloomTex;
uniform float2    rcpFrame;
uniform float     bloomIntensity;

// Quality preset 12: 5-step search (1.5, 2.0, 2.0, 2.0, 8.0)
static const float FXAA_SUBPIX             = 0.75;
static const float FXAA_EDGE_THRESHOLD     = 0.125;
static const float FXAA_EDGE_THRESHOLD_MIN = 0.0625;
static const float steps[5]               = { 1.5, 2.0, 2.0, 2.0, 8.0 };

// Green-channel-weighted luma (Lottes formula — blue excluded)
float fxaaLuma(float3 rgb) { return rgb.y * (0.587 / 0.299) + rgb.x; }

float4 fxaaPass(float2 uv)
{
    float4 rgbaM = screenTexture.Sample(uv);
    float  lumaM = fxaaLuma(rgbaM.rgb);

    float lumaN  = fxaaLuma(screenTexture.Sample(uv + float2( 0,  rcpFrame.y)).rgb);
    float lumaS  = fxaaLuma(screenTexture.Sample(uv + float2( 0, -rcpFrame.y)).rgb);
    float lumaE  = fxaaLuma(screenTexture.Sample(uv + float2( rcpFrame.x,  0)).rgb);
    float lumaW  = fxaaLuma(screenTexture.Sample(uv + float2(-rcpFrame.x,  0)).rgb);

    float rangeMax = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));
    float rangeMin = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));
    float range    = rangeMax - rangeMin;

    if (range < max(FXAA_EDGE_THRESHOLD_MIN, rangeMax * FXAA_EDGE_THRESHOLD))
    {
        return rgbaM;
    }

    float lumaNW = fxaaLuma(screenTexture.Sample(uv + float2(-rcpFrame.x,  rcpFrame.y)).rgb);
    float lumaNE = fxaaLuma(screenTexture.Sample(uv + float2( rcpFrame.x,  rcpFrame.y)).rgb);
    float lumaSW = fxaaLuma(screenTexture.Sample(uv + float2(-rcpFrame.x, -rcpFrame.y)).rgb);
    float lumaSE = fxaaLuma(screenTexture.Sample(uv + float2( rcpFrame.x, -rcpFrame.y)).rgb);

    float subpixNSEW = lumaN + lumaS + lumaE + lumaW;
    float subpixA    = (2.0 * subpixNSEW + lumaNW + lumaNE + lumaSW + lumaSE) / 12.0;
    float subpixB    = clamp(abs(subpixA - lumaM) / range, 0.0, 1.0);
    float subpixC    = (-2.0 * subpixB + 3.0) * subpixB * subpixB;
    float subpixD    = subpixC * subpixC * FXAA_SUBPIX;

    float edgeHorz =
        abs(lumaNW + -2.0 * lumaN + lumaNE) +
        abs(lumaW  + -2.0 * lumaM + lumaE ) * 2.0 +
        abs(lumaSW + -2.0 * lumaS + lumaSE);
    float edgeVert =
        abs(lumaNW + -2.0 * lumaW + lumaSW) +
        abs(lumaN  + -2.0 * lumaM + lumaS ) * 2.0 +
        abs(lumaNE + -2.0 * lumaE + lumaSE);
    bool horzSpan = (edgeHorz >= edgeVert);

    float luma1 = horzSpan ? lumaN : lumaW;
    float luma2 = horzSpan ? lumaS : lumaE;
    float grad1 = luma1 - lumaM;
    float grad2 = luma2 - lumaM;
    bool  pairN = abs(grad1) >= abs(grad2);

    float gradientScaled = 0.25 * max(abs(grad1), abs(grad2));
    float stepLen        = horzSpan ? rcpFrame.y : rcpFrame.x;
    if (pairN)
    {
        stepLen = -stepLen;
    }
    float localAvg       = 0.5 * (pairN ? luma1 : luma2) + 0.5 * lumaM;

    float2 posP  = uv;
    if (horzSpan)
    {
        posP.y += stepLen * 0.5;
    }
    else
    {
        posP.x += stepLen * 0.5;
    }

    float2 offNP = horzSpan ? float2(rcpFrame.x, 0.0) : float2(0.0, rcpFrame.y);

    float2 posN      = posP - offNP * steps[0];
    float2 posPos    = posP + offNP * steps[0];
    float  lumaEndN  = fxaaLuma(screenTexture.Sample(posN  ).rgb) - localAvg;
    float  lumaEndP  = fxaaLuma(screenTexture.Sample(posPos).rgb) - localAvg;
    bool   doneN     = abs(lumaEndN) >= gradientScaled;
    bool   doneP     = abs(lumaEndP) >= gradientScaled;

    for (int i = 1; i < 5; i++)
    {
        if (!doneN) {
            posN    -= offNP * steps[i];
            lumaEndN = fxaaLuma(screenTexture.Sample(posN  ).rgb) - localAvg;
            doneN    = abs(lumaEndN) >= gradientScaled;
        }
        if (!doneP) {
            posPos  += offNP * steps[i];
            lumaEndP = fxaaLuma(screenTexture.Sample(posPos).rgb) - localAvg;
            doneP    = abs(lumaEndP) >= gradientScaled;
        }
        if (doneN && doneP)
        {
            break;
        }
    }

    float dstN      = horzSpan ? (uv.x - posN.x  ) : (uv.y - posN.y  );
    float dstP      = horzSpan ? (posPos.x - uv.x ) : (posPos.y - uv.y);
    bool  goodSpanN = (lumaEndN < 0.0) != (lumaM < localAvg);
    bool  goodSpanP = (lumaEndP < 0.0) != (lumaM < localAvg);
    float spanLen   = dstN + dstP;

    float pixelOffset = max(
        goodSpanN ? (0.5 - dstN / spanLen) : 0.0,
        goodSpanP ? (0.5 - dstP / spanLen) : 0.0
    );
    pixelOffset = max(pixelOffset, subpixD);

    float2 finalUV = uv;
    if (horzSpan)
    {
        finalUV.y += pixelOffset * stepLen;
    }
    else
    {
        finalUV.x += pixelOffset * stepLen;
    }

    return screenTexture.Sample(finalUV);
}

struct FSInput { float2 texCoords : TEXCOORD0; }

[shader("fragment")]
float4 fragMain(FSInput input, float4 fragCoord : SV_Position) : SV_Target
{
    float4 color = fxaaPass(input.texCoords);
    if (bloomIntensity > 0.0)
    {
        color.rgb += bloomTex.Sample(input.texCoords).rgb * bloomIntensity;
    }
    color.rgb = dither8(color.rgb, fragCoord.xy);
    return color;
}
