#pragma once
#include "lighting.slang"
#include "shadows.slang"

// Per-program uniform blocks shared by both the vertex and fragment stage.
//
// Each stage is compiled by a separate slangc invocation, so loose `uniform`
// globals get collected into a per-stage block containing only the fields that
// stage references, which makes the vertex and fragment blocks disagree and
// fails program linking ("struct type mismatch ... block_GlobalParams").
//
// A named ConstantBuffer is emitted whole (no per-field stripping) and with an
// identical layout in every stage that references it, so both stages agree.
// The explicit binding keeps the block binding equal across stages too (the
// engine still overrides it via glUniformBlockBinding at index 0).
//
// Slang may append _N suffixes to disambiguate identically-named fields across
// structs; the engine's UBO introspection strips these suffixes so C++ setX()
// calls use the original names. Unreferenced ConstantBuffers are stripped by
// slangc, so a shader only emits the block it actually uses.

struct PbrParams {
    float4x4 mvp;           // vertex
    float4x4 model;         // vertex
    float4x4 normalMatrix;  // vertex; precomputed transpose(inverse(model)) on CPU
    float4x4 lightSpaceMatrices[maxCascades];
    int      cascadeCount;
    float    metallicFactor;
    float    roughnessFactor;
    float    clearcoatFactor;  // 0 = no coat
    float    clearcoatRoughnessFactor;
    float4   baseColorFactor;      // linear; multiplies the albedo texture, or is the albedo with none
    float    alphaCutoff;          // 0 = opaque; else discard where albedo alpha is below it
    bool     alphaBlend;           // glTF BLEND: output premultiplied alpha
    float4   diffuseTransmission;  // rgb = color, w = factor; 0 = opaque
    float    ao;
    bool     ssaoEnabled;
    float    ambientStrength;
    float    evsmBleedThreshold;
    bool     hasNoTexture;
    bool     hasNormalMap;
    bool     hasAOMap;
    bool     hasEmissiveMap;
    bool     hasMetallicRoughnessMap;
    bool     hasDiffuseTransmissionMap;
    // xy = offset, zw = scale (KHR_texture_transform; no rotation support)
    float4           albedoUVTransform;
    float4           normalUVTransform;
    float4           occlusionUVTransform;
    float4           emissiveUVTransform;
    float4           metallicRoughnessUVTransform;
    float4           diffuseTransmissionUVTransform;
    float3           viewPos;
    float3           emissive;
    DirectionalLight directionalLight;
    int              numLights;
    int              attenuationIndex;  // shared by all point lights
    float            clusterNear;       // near bound of the cluster grid
    float            farPlane;
    float2           screenSize;
    float3           viewForward;    // world-space camera forward, normalized
    bool             probeEnabled;   // sample probeMap for the ambient specular term
    float3           probePosition;  // world-space capture point
    float3           probeBoxMin;    // world-space box the probe is projected against
    float3           probeBoxMax;
    float            probeMaxMip;  // last mip of probeMap; roughness 1 samples it
};
[[vk::binding(2)]] ConstantBuffer<PbrParams> params;
