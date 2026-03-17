#version 330 core

// FXAA 3.11 by Timothy Lottes, NVIDIA Corporation
// To the extent possible under law, the author(s) have dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide. This software is distributed without any warranty.
// See <http://creativecommons.org/publicdomain/zero/1.0/> for details.
//
// Adapted for GLSL 3.30 core profile.

in  vec2 fragTexCoords;
out vec4 fragColor;

uniform sampler2D screenTexture;
uniform vec2      rcpFrame;

// ---------------------------------------------------------------------------
// Quality preset 12: 5-step search (step sizes 1.5, 2.0, 2.0, 2.0, 8.0).
// Chosen because it eliminates stairstepping on geometry edges without the
// cost of preset 29 (12-step search), which is overkill for real-time use.
// ---------------------------------------------------------------------------
#define FXAA_SUBPIX             0.75
// Skip pixels whose contrast is below this fraction of local peak luma
// (0.125 = 1/8 — tolerant enough to catch most visible edges).
#define FXAA_EDGE_THRESHOLD     0.125
// Absolute floor so FXAA never fires on near-black content where even
// large relative contrast represents imperceptible differences.
#define FXAA_EDGE_THRESHOLD_MIN 0.0625

// ---------------------------------------------------------------------------
// Green-channel-weighted luma per the original Lottes formula.
// Blue is excluded because human contrast sensitivity in that channel is
// negligible for edge-detection purposes, buying a small speed gain.
// ---------------------------------------------------------------------------
float fxaaLuma(vec3 rgb) {
    return rgb.y * (0.587 / 0.299) + rgb.x;
}

vec4 fxaaPass(vec2 uv) {
    vec4  rgbaM = texture(screenTexture, uv);
    float lumaM = fxaaLuma(rgbaM.rgb);

    float lumaN = fxaaLuma(textureOffset(screenTexture, uv, ivec2( 0,  1)).rgb);
    float lumaS = fxaaLuma(textureOffset(screenTexture, uv, ivec2( 0, -1)).rgb);
    float lumaE = fxaaLuma(textureOffset(screenTexture, uv, ivec2( 1,  0)).rgb);
    float lumaW = fxaaLuma(textureOffset(screenTexture, uv, ivec2(-1,  0)).rgb);

    float rangeMax = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));
    float rangeMin = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));
    float range    = rangeMax - rangeMin;

    if (range < max(FXAA_EDGE_THRESHOLD_MIN, rangeMax * FXAA_EDGE_THRESHOLD)) {
        return rgbaM;
    }

    float lumaNW = fxaaLuma(textureOffset(screenTexture, uv, ivec2(-1,  1)).rgb);
    float lumaNE = fxaaLuma(textureOffset(screenTexture, uv, ivec2( 1,  1)).rgb);
    float lumaSW = fxaaLuma(textureOffset(screenTexture, uv, ivec2(-1, -1)).rgb);
    float lumaSE = fxaaLuma(textureOffset(screenTexture, uv, ivec2( 1, -1)).rgb);

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
    if (pairN) stepLen   = -stepLen;
    float localAvg       = 0.5 * (pairN ? luma1 : luma2) + 0.5 * lumaM;

    vec2 posP = uv;
    if (horzSpan) posP.y += stepLen * 0.5;
    else          posP.x += stepLen * 0.5;

    vec2 offNP = horzSpan ? vec2(rcpFrame.x, 0.0) : vec2(0.0, rcpFrame.y);

    // Quality preset 12 search step distances: 1.5, 2.0, 2.0, 2.0, 8.0
    const float steps[5] = float[5](1.5, 2.0, 2.0, 2.0, 8.0);

    vec2  posN     = posP - offNP * steps[0];
    vec2  posPos   = posP + offNP * steps[0];
    float lumaEndN = fxaaLuma(texture(screenTexture, posN  ).rgb) - localAvg;
    float lumaEndP = fxaaLuma(texture(screenTexture, posPos).rgb) - localAvg;
    bool  doneN    = abs(lumaEndN) >= gradientScaled;
    bool  doneP    = abs(lumaEndP) >= gradientScaled;

    for (int i = 1; i < 5; i++) {
        if (!doneN) {
            posN    -= offNP * steps[i];
            lumaEndN = fxaaLuma(texture(screenTexture, posN  ).rgb) - localAvg;
            doneN    = abs(lumaEndN) >= gradientScaled;
        }
        if (!doneP) {
            posPos  += offNP * steps[i];
            lumaEndP = fxaaLuma(texture(screenTexture, posPos).rgb) - localAvg;
            doneP    = abs(lumaEndP) >= gradientScaled;
        }
        if (doneN && doneP) break;
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

    vec2 finalUV = uv;
    if (horzSpan) finalUV.y += pixelOffset * stepLen;
    else          finalUV.x += pixelOffset * stepLen;

    return texture(screenTexture, finalUV);
}

void main() {
    fragColor = fxaaPass(fragTexCoords);
}
