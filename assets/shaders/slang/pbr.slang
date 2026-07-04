// pbr.slang: PBR vertex + fragment shader
#include "include/uniforms.slang"
#include "include/shadows.slang"
#include "include/clustered.slang"
#include "include/dither.slang"

struct PointLightGPU {
    float3 color;
    float  pad0;
    float3 position;
    float  pad1;
};

[[vk::binding(3)]] StructuredBuffer<PointLightGPU> lightBuffer;
[[vk::binding(4)]] StructuredBuffer<uint2>         lightGrid;
[[vk::binding(5)]] StructuredBuffer<uint>          lightIndices;

struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
};

struct VSOutput {
    float4 pos : SV_Position;
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float2 texCoords : TEXCOORD2;
    float4 fragPosLightSpace : TEXCOORD3;
};

// clang-format off
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    // clang-format on
    VSOutput output;
    float4   worldPos4       = mul(params.model, float4(input.position, 1.0));
    output.pos               = mul(params.mvp, float4(input.position, 1.0));
    output.worldPosition     = worldPos4.xyz;
    output.worldNormal       = mul((float3x3)params.normalMatrix, input.normal);
    output.texCoords         = input.texCoord;
    output.fragPosLightSpace = mul(params.lightSpaceMatrix, worldPos4);
    return output;
};

[[vk::binding(0)]] Sampler2D<float4> texture_diffuse1;
[[vk::binding(1)]] Sampler2D<float2> shadowMap;

// clang-format off
[shader("fragment")]
float4 fragMain(VSOutput input, float4 fragCoord : SV_Position) : SV_Target {
    // clang-format on
    float3 texColor = pow(texture_diffuse1.Sample(input.texCoords).rgb, float3(2.2));
    float3 albedo   = lerp(texColor, float3(1.0), float(params.hasNoTexture));

    float3 N  = normalize(input.worldNormal);
    float3 V  = normalize(params.viewPos - input.worldPosition);
    float3 Lo = float3(0.0);

    // Directional light
    float3 dirContrib = evaluatePBR(params.directionalLight, input.worldPosition, albedo, N, V, params.metallic, params.roughness);
    float  shadow     = 0.0;
    if (params.directionalLight.castShadow) {
        shadow = shadowEVSM(shadowMap, input.fragPosLightSpace, params.evsmBleedThreshold);
    }
    Lo += dirContrib * (1.0 - shadow) * float(params.directionalLight.enabled);

    // Clustered point lights
    if (params.numLights > 0) {
        float viewZ   = dot(input.worldPosition - params.viewPos,
                            params.viewForward);
        int   ci      = clusterIndex(fragCoord.xy, viewZ,
                                     params.screenSize,
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
            Lo += evaluatePBR(pl, input.worldPosition, albedo, N, V,
                              params.metallic, params.roughness);
        }
    }

    float3 ambient = float3(params.ambientStrength) * albedo * params.ao;
    float3 color   = ambient + Lo;

    // Reinhard HDR tone mapping
    color = color / (color + float3(1.0));

    // Gamma correction
    color = pow(color, float3(1.0 / 2.2));

    color = dither8(color, fragCoord.xy);

    return float4(color + params.emissive, 1.0);
};
