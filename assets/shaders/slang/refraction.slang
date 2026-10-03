// refraction.slang: screen-space refraction for objects flagged in the scene.
//
// The opaque color has already been copied. This shader samples that copy,
// never the attachment it is writing. Thickness is a constant, so the tint
// is a constant too. Roughness is the PBR floor: this pass does not read
// the mesh roughness map.
#include "include/clustered.slang"
#include "include/shadows.slang"

struct PointLightGPU {
    float3 color;
    float  pad0;
    float3 position;
    float  pad1;
};

[[vk::binding(3)]] StructuredBuffer<PointLightGPU> lightBuffer;
[[vk::binding(4)]] StructuredBuffer<uint2>         lightGrid;
[[vk::binding(5)]] StructuredBuffer<uint>          lightIndices;

struct RefractionParams {
    float4x4         mvp;
    float4x4         model;
    float4x4         normalMatrix;
    float4x4         lightSpaceMatrices[maxCascades];
    int              cascadeCount;
    float4x4         mvpNoJitter;
    float4x4         prevMvpNoJitter;
    float4x4         cameraMVP;
    float3           viewPos;
    float3           viewForward;
    float2           screenSize;
    float            clusterNear;
    float            farPlane;
    int              numLights;
    int              attenuationIndex;
    float            ior;
    float            thickness;
    float3           tint;
    float            evsmBleedThreshold;
    DirectionalLight directionalLight;
    bool             probeEnabled;
    float3           probePosition;
    float3           probeBoxMin;
    float3           probeBoxMax;
    float            probeMaxMip;
};
[[vk::binding(2)]] ConstantBuffer<RefractionParams> params;

// Same VSInput layout as pbr.slang so the mesh VAO locations match.
// texCoord and tangent are unused: this pass does not sample material maps.
struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

struct VSOutput {
    float4 pos : SV_Position;
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float4 currClip : TEXCOORD3;
    float4 prevClip : TEXCOORD4;
};

struct FSOutput {
    float4 color : SV_Target0;
    float2 velocity : SV_Target1;
};

[[vk::binding(1)]] Sampler2DArray<float4> shadowMap;
[[vk::binding(11)]] Sampler2D<float4>     sceneColor;
[[vk::binding(12)]] Sampler2D<float>      opaqueDepth;
[[vk::binding(13)]] SamplerCube<float4>   probeMap;

static const float glassRoughness = 0.089;

// clang-format off
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    // clang-format on
    VSOutput o;
    float4   localPos  = float4(input.position, 1.0);
    float4   worldPos4 = mul(params.model, localPos);
    o.pos              = mul(params.mvp, localPos);
    o.worldPosition    = worldPos4.xyz;
    o.worldNormal      = mul((float3x3)params.normalMatrix, input.normal);
    o.currClip         = mul(params.mvpNoJitter, localPos);
    o.prevClip         = mul(params.prevMvpNoJitter, localPos);
    return o;
};

// clang-format off
[shader("fragment")]
FSOutput fragMain(VSOutput input, float4 fragCoord : SV_Position) {
    // clang-format on
    FSOutput o;
    float2   currUV = input.currClip.xy / input.currClip.w * 0.5 + 0.5;
    float2   prevUV = input.prevClip.xy / input.prevClip.w * 0.5 + 0.5;
    o.velocity      = currUV - prevUV;

    float  nLen = length(input.worldNormal);
    float3 N    = nLen > 1e-4 ? input.worldNormal / nLen : float3(0.0, 1.0, 0.0);
    float3 V    = normalize(params.viewPos - input.worldPosition);

    // Incident points from the camera toward the surface. Eta is air into
    // the object. refract returns 0 on total internal reflection.
    float3 R           = refract(-V, N, 1.0 / params.ior);
    float  transmitted = dot(R, R) > 1e-6 ? 1.0 : 0.0;
    float3 exitPos     = input.worldPosition + R * params.thickness;
    float4 exitClip    = mul(params.cameraMVP, float4(exitPos, 1.0));
    // w <= 0 means the exit point is behind the camera. Sending that uv
    // through the screen fade is enough; the divide itself would be Inf.
    float  behindCamera = exitClip.w > 0.0 ? 1.0 : 0.0;
    float2 exitUV       = behindCamera > 0.0 ? exitClip.xy / exitClip.w * 0.5 + 0.5 :
                                               float2(-1.0, -1.0);

    float2 edge       = smoothstep(float2(0.0), float2(0.02), exitUV) *
                        smoothstep(float2(0.0), float2(0.02), 1.0 - exitUV);
    float  screenFade = edge.x * edge.y;
    // Compare against the glass surface, not the exit point. The exit is
    // often past the first thing the ray should show, and that rejection
    // replaces the bend with the straight sample.
    float sceneDepth = opaqueDepth.Sample(exitUV);
    float glassDepth = fragCoord.z;
    float depthFade =
        smoothstep(glassDepth - 0.005, glassDepth + 0.005, sceneDepth);
    float weight = transmitted * screenFade * depthFade * behindCamera;

    float2 fragUV     = fragCoord.xy / params.screenSize;
    float3 straight   = sceneColor.Sample(fragUV).rgb;
    float3 bent       = sceneColor.Sample(exitUV).rgb;
    float3 refraction = lerp(straight, bent, weight) * params.tint;

    float  f0 = pow((params.ior - 1.0) / (params.ior + 1.0), 2.0);
    float3 F0 = float3(f0);

    float3 spec    = float3(0.0);
    float3 dirSpec = evaluateSpecular(params.directionalLight, input.worldPosition,
                                      N, V, F0, glassRoughness);
    float  shadow  = 0.0;
    if (params.directionalLight.castShadow) {
        shadow = shadowCascaded(shadowMap, params.lightSpaceMatrices,
                                params.cascadeCount, input.worldPosition,
                                params.evsmBleedThreshold);
    }
    spec += dirSpec * (1.0 - shadow) * float(params.directionalLight.enabled);

    if (params.numLights > 0) {
        float viewZ   = dot(input.worldPosition - params.viewPos,
                            params.viewForward);
        int   ci      = clusterIndex(fragCoord.xy, viewZ, params.screenSize,
                                     params.clusterNear, params.farPlane);
        uint2 tile    = lightGrid[ci];
        uint  tileEnd = tile.x + tile.y;
        for (uint j = tile.x; j < tileEnd; ++j) {
            uint          li  = lightIndices[j];
            PointLightGPU gpu = lightBuffer[li];
            PointLight    pl;
            pl.color            = gpu.color;
            pl.position         = gpu.position;
            pl.attenuationIndex = params.attenuationIndex;
            spec += evaluateSpecular(pl, input.worldPosition, N, V, F0,
                                     glassRoughness);
        }
    }

    // View Fresnel splits the surface between the probe reflection and the
    // refraction, as pbr.slang splits ambient. With no probe there is nothing
    // to reflect, and the factor would only darken the edges, so it is not
    // applied.
    if (params.probeEnabled) {
        float  NdotV = max(dot(N, V), 1e-5);
        float3 F     = fresnelSchlickRoughness(NdotV, F0, glassRoughness);
        float2 AB    = envBRDFApprox(NdotV, glassRoughness);
        float3 Rv    = boxProject(reflect(-V, N), input.worldPosition,
                                  params.probePosition, params.probeBoxMin,
                                  params.probeBoxMax);
        float3 probe = probeMap.SampleLevel(Rv, glassRoughness * params.probeMaxMip).rgb;
        refraction   = refraction * (1.0 - F) + probe * (F0 * AB.x + AB.y);
    }
    o.color = float4(refraction + spec, 1.0);
    return o;
};
