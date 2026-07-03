#pragma once
#include "lighting.slang"

// Per-program uniform blocks shared by both the vertex and fragment stage.
//
// Each stage is compiled by a separate slangc invocation, so loose `uniform`
// globals get collected into a per-stage block containing only the fields that
// stage references — which makes the vertex and fragment blocks disagree and
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

struct PbrParams
{
    float4x4         mvp;                // vertex
    float4x4         model;              // vertex
    float4x4         normalMatrix;       // vertex; precomputed transpose(inverse(model)) on CPU
    float4x4         lightSpaceMatrix;  // vertex
    float            metallic;
    float            roughness;
    float            ao;
    float            ambientStrength;
    float            evsmBleedThreshold;
    bool             hasNoTexture;
    float3           viewPos;
    float3           emissive;
    DirectionalLight directionalLight;
    int              numLights;
    float            clusterNear;  // near bound of the cluster grid
    float            farPlane;
    float2           screenSize;
    float3           viewForward;  // world-space camera forward, normalized
    int              tilesZ;       // runtime cluster z-slice count
};
[[vk::binding(2)]] ConstantBuffer<PbrParams> params;
