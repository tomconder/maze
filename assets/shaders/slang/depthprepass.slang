// depthprepass.slang — camera-space depth-only prepass + active-tile marking
#include "include/clustered.slang"

uniform float4x4 mvp;
uniform float4x4 modelView;

// Fragment-stage params in a named ConstantBuffer so the vertex and fragment
// loose-uniform blocks cannot disagree (see uniforms.slang).
struct PrepassParams
{
    float2 screenSize;
    float  clusterNear;
    float  clusterFar;
    int    tilesZ;
};
[[vk::binding(2)]] ConstantBuffer<PrepassParams> prepassParams;

// Active volume-tile flags, one uint per cluster — consumed by
// cluster_assign.slang so light assignment skips clusters with no fragments.
[[vk::binding(8)]] RWStructuredBuffer<uint> tileFlags;

struct VSInput  { float3 position : POSITION; }
struct VSOutput
{
    float4 pos   : SV_Position;
    float  viewZ : TEXCOORD0;  // positive view-space depth
}

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput o;
    o.pos   = mul(mvp, float4(input.position, 1.0));
    o.viewZ = -mul(modelView, float4(input.position, 1.0)).z;
    return o;
}

[shader("fragment")]
void fragMain(VSOutput input, float4 fragCoord : SV_Position)
{
    int ci = clusterIndex(fragCoord.xy, input.viewZ,
                          prepassParams.screenSize, prepassParams.clusterNear,
                          prepassParams.clusterFar, prepassParams.tilesZ);
    tileFlags[ci] = 1U;
}
