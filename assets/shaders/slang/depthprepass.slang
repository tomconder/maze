// depthprepass.slang: camera-space depth-only prepass, plus the screen-space
// velocity buffer TAA resolves through.
uniform float4x4 mvp;              // jittered; must match the scene pass
uniform float4x4 mvpNoJitter;      // this frame, unjittered
uniform float4x4 prevMvpNoJitter;  // last frame, unjittered

struct VSInput {
    float3 position : POSITION;
};
struct VSOutput {
    float4 pos : SV_Position;
    // Unjittered clip positions: motion is measured between pixel centres, so
    // the sub-pixel jitter must not enter the difference.
    float4 currClip : TEXCOORD0;
    float4 prevClip : TEXCOORD1;
};

// clang-format off
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    // clang-format on
    VSOutput o;
    float4   position = float4(input.position, 1.0);
    o.pos             = mul(mvp, position);
    o.currClip        = mul(mvpNoJitter, position);
    o.prevClip        = mul(prevMvpNoJitter, position);
    return o;
};

// clang-format off
[shader("fragment")]
float2 fragMain(VSOutput input) : SV_Target {
    // clang-format on
    float2 currUV = input.currClip.xy / input.currClip.w * 0.5 + 0.5;
    float2 prevUV = input.prevClip.xy / input.prevClip.w * 0.5 + 0.5;
    return currUV - prevUV;
};
