// depthprepass.slang: camera-space depth-only prepass, plus the screen-space
// velocity buffer TAA resolves through and the view-space normal buffer SSAO
// reads.
uniform float4x4 mvp;              // jittered; must match the scene pass
uniform float4x4 mvpNoJitter;      // this frame, unjittered
uniform float4x4 prevMvpNoJitter;  // last frame, unjittered
uniform float4x4 normalMatrix;     // transpose(inverse(view * model))

// Same VSInput layout as pbr.slang (position/texCoord/normal/tangent), so
// slangc assigns the same attribute locations the mesh VAO was built
// against. texCoord/tangent go unused here. Light cubes draw through this
// shader too but their VAO only has position bound (see cube.cpp), so their
// normal reads the GL default (0,0,0) — filtered out downstream by the
// zero-length check in ssao.slang.
struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};
struct VSOutput {
    float4 pos : SV_Position;
    // Unjittered clip positions: motion is measured between pixel centres, so
    // the sub-pixel jitter must not enter the difference.
    float4 currClip : TEXCOORD0;
    float4 prevClip : TEXCOORD1;
    float3 viewNormal : TEXCOORD2;
};
struct FSOutput {
    float2 velocity : SV_Target0;
    float4 normal : SV_Target1;
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
    o.viewNormal      = mul((float3x3)normalMatrix, input.normal);
    return o;
};

// clang-format off
[shader("fragment")]
FSOutput fragMain(VSOutput input) {
    // clang-format on
    FSOutput o;
    float2   currUV = input.currClip.xy / input.currClip.w * 0.5 + 0.5;
    float2   prevUV = input.prevClip.xy / input.prevClip.w * 0.5 + 0.5;
    o.velocity      = currUV - prevUV;

    // Guard against normalizing a true-zero normal (light cubes / geometry
    // with no NORMAL attribute bound): 0/0 is NaN, and a NaN sentinel breaks
    // the "no surface here" length check ssao.slang relies on.
    float len = length(input.viewNormal);
    o.normal  = float4(len > 1e-4 ? input.viewNormal / len : float3(0.0), 0.0);
    return o;
};
