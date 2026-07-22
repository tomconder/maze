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
    float4 tangent : TANGENT;
};

struct VSOutput {
    float4 pos : SV_Position;
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float2 texCoords : TEXCOORD2;
    float4 fragPosLightSpace : TEXCOORD3;
    float4 worldTangent : TEXCOORD4;
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
    output.worldTangent      = float4(mul((float3x3)params.model, input.tangent.xyz), input.tangent.w);
    return output;
};

[[vk::binding(0)]] Sampler2D<float4> texture_diffuse1;
[[vk::binding(1)]] Sampler2D<float2> shadowMap;
[[vk::binding(6)]] Sampler2D<float4> normalMap;
[[vk::binding(7)]] Sampler2D<float4> occlusionMap;
[[vk::binding(8)]] Sampler2D<float4> emissiveMap;
[[vk::binding(9)]] Sampler2D<float4> metallicRoughnessMap;

// KHR_texture_transform: t.xy = offset, t.zw = scale (no rotation support)
float2 applyUVTransform(float2 uv, float4 t) {
    return uv * t.zw + t.xy;
}

// clang-format off
[shader("fragment")]
float4 fragMain(VSOutput input, float4 fragCoord : SV_Position) : SV_Target {
    // clang-format on
    float2 albedoUV = applyUVTransform(input.texCoords, params.albedoUVTransform);
    float3 texColor = pow(texture_diffuse1.Sample(albedoUV).rgb, float3(2.2));
    float3 albedo   = lerp(texColor, float3(1.0), float(params.hasNoTexture));

    float3 N = normalize(input.worldNormal);
    if (params.hasNormalMap) {
        float2 normalUV      = applyUVTransform(input.texCoords, params.normalUVTransform);
        float3 T             = normalize(input.worldTangent.xyz - N * dot(N, input.worldTangent.xyz));
        float3 B             = cross(N, T) * input.worldTangent.w;
        float3 sampledNormal = normalMap.Sample(normalUV).rgb * 2.0 - 1.0;
        N                    = normalize(T * sampledNormal.x + B * sampledNormal.y + N * sampledNormal.z);
    }

    float ao = params.ao;
    if (params.hasAOMap) {
        float2 occlusionUV = applyUVTransform(input.texCoords, params.occlusionUVTransform);
        ao *= occlusionMap.Sample(occlusionUV).r;
    }

    float3 emissiveTexColor = float3(0.0);
    if (params.hasEmissiveMap) {
        float2 emissiveUV = applyUVTransform(input.texCoords, params.emissiveUVTransform);
        emissiveTexColor  = pow(emissiveMap.Sample(emissiveUV).rgb, float3(2.2));
    }

    // glTF convention: G = roughness, B = metallic
    float metallic  = params.metallicFactor;
    float roughness = params.roughnessFactor;
    if (params.hasMetallicRoughnessMap) {
        float2 metallicRoughnessUV = applyUVTransform(input.texCoords, params.metallicRoughnessUVTransform);
        float3 mr                  = metallicRoughnessMap.Sample(metallicRoughnessUV).rgb;
        metallic *= mr.b;
        roughness *= mr.g;
    }
    roughness = clamp(roughness, 0.089, 1.0);

    float3 V  = normalize(params.viewPos - input.worldPosition);
    float3 Lo = float3(0.0);

    // Directional light
    float3 dirContrib = evaluatePBR(params.directionalLight, input.worldPosition, albedo, N, V, metallic, roughness);
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
                              metallic, roughness);
        }
    }

    float3 ambient = float3(params.ambientStrength) * albedo * ao;
    float3 color   = ambient + Lo;

    // Reinhard HDR tone mapping
    color = color / (color + float3(1.0));

    // Gamma correction
    color = pow(color, float3(1.0 / 2.2));

    color = dither8(color, fragCoord.xy);

    return float4(color + params.emissive + emissiveTexColor, 1.0);
};
