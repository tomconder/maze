// depthprepass.slang: camera-space depth-only prepass, plus the screen-space
// velocity buffer TAA resolves through and the view-space normal buffer SSAO
// reads. The normal alpha holds the reflection strength SSR reads.
#include "include/instance.slang"

uniform float4x4 mvp;              // jittered; must match the scene pass
uniform float4x4 mvpNoJitter;      // this frame, unjittered
uniform float4x4 prevMvpNoJitter;  // last frame, unjittered
uniform float4x4 normalMatrix;     // transpose(inverse(view * model))
uniform float    reflectivity;     // 0 = no reflection; read by ssr.slang
uniform float    cubeScale;        // vertCubeMain only
// Light cubes, one instance each: three float4 (position, previous position,
// colour). Must match Cube::Instance in cube.hpp. vertCubeMain only.
[[vk::binding(8)]] StructuredBuffer<float4> cubeInstances;
// glTF MASK, the same test as pbr.slang so the prepass depth matches the scene
// pass. Only fragMainMasked reads these; it is its own program because a
// discard turns off early-Z for every draw of the program. The albedo texture
// is on unit 0.
uniform float                        alphaCutoff;     // 0 = opaque, no test
uniform float                        baseColorAlpha;  // baseColorFactor.a
uniform bool                         hasNoTexture;
uniform float4                       albedoUVTransform;  // xy = offset, zw = scale
[[vk::binding(0)]] Sampler2D<float4> albedoMap;

// Same VSInput layout as pbr.slang (position/texCoord/normal/tangent), so
// slangc assigns the same attribute locations the mesh VAO was built
// against. texCoord/tangent go unused here. Light cubes use vertCubeMain
// below: they have no normal, so theirs is (0,0,0) — filtered out downstream by the
// zero-length check in ssao.slang. That zero normal is written here as
// float4(0), so its alpha is 0 too, and ssr.slang skips those pixels.
struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};
struct VSOutput {
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD3;
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
    o.texCoord        = input.texCoord;
    return o;
};

// The light cubes: the position-only cube VAO, drawn instanced.
struct CubeVSInput {
    float3 position : POSITION;
};

// clang-format off
[shader("vertex")]
VSOutput vertCubeMain(CubeVSInput input) {
    // clang-format on
    VSOutput o;
    uint     instance = instanceIndex();
    float3   scaled   = input.position * cubeScale;
    float4   position = float4(scaled + cubeInstances[instance * 3].xyz, 1.0);
    float4   previous = float4(scaled + cubeInstances[instance * 3 + 1].xyz, 1.0);
    o.pos             = mul(mvp, position);
    o.currClip        = mul(mvpNoJitter, position);
    o.prevClip        = mul(prevMvpNoJitter, previous);
    o.viewNormal      = float3(0.0);
    o.texCoord        = float2(0.0);
    return o;
};

FSOutput shade(VSOutput input, bool frontFacing) {
    FSOutput o;
    float2   currUV = input.currClip.xy / input.currClip.w * 0.5 + 0.5;
    float2   prevUV = input.prevClip.xy / input.prevClip.w * 0.5 + 0.5;
    o.velocity      = currUV - prevUV;

    // Guard against normalizing a true-zero normal (light cubes / geometry
    // with no NORMAL attribute bound): 0/0 is NaN, and a NaN sentinel breaks
    // the "no surface here" length check ssao.slang relies on.
    // A back face is only drawn for a double-sided mesh; SSAO and SSR want the
    // normal on the side the camera sees.
    float3 viewNormal = frontFacing ? input.viewNormal : -input.viewNormal;
    float  len        = length(viewNormal);
    o.normal          = len > 1e-4 ? float4(viewNormal / len, reflectivity) : float4(0.0);
    return o;
};

// clang-format off
[shader("fragment")]
FSOutput fragMain(VSOutput input, bool frontFacing : SV_IsFrontFace) {
    // clang-format on
    return shade(input, frontFacing);
};

// clang-format off
[shader("fragment")]
FSOutput fragMainMasked(VSOutput input, bool frontFacing : SV_IsFrontFace) {
    // clang-format on
    float2 uv    = input.texCoord * albedoUVTransform.zw + albedoUVTransform.xy;
    float  alpha = (hasNoTexture ? 1.0 : albedoMap.Sample(uv).a) * baseColorAlpha;
    if (alpha < alphaCutoff) {
        discard;
    }
    return shade(input, frontFacing);
};
