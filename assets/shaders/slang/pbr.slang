// pbr.slang: PBR vertex + fragment shader
#include "include/uniforms.slang"
#include "include/shadows.slang"
#include "include/clustered.slang"

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
    float4 worldTangent : TEXCOORD4;
};

// clang-format off
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    // clang-format on
    VSOutput output;
    float4   worldPos4   = mul(params.model, float4(input.position, 1.0));
    output.pos           = mul(params.mvp, float4(input.position, 1.0));
    output.worldPosition = worldPos4.xyz;
    output.worldNormal   = mul((float3x3)params.normalMatrix, input.normal);
    output.texCoords     = input.texCoord;
    output.worldTangent  = float4(mul((float3x3)params.model, input.tangent.xyz), input.tangent.w);
    return output;
};

[[vk::binding(0)]] Sampler2D<float4>      texture_diffuse1;
[[vk::binding(1)]] Sampler2DArray<float4> shadowMap;
[[vk::binding(6)]] Sampler2D<float4>      normalMap;
[[vk::binding(7)]] Sampler2D<float4>      occlusionMap;
[[vk::binding(8)]] Sampler2D<float4>      emissiveMap;
[[vk::binding(9)]] Sampler2D<float4>      metallicRoughnessMap;
[[vk::binding(10)]] Sampler2D<float>      ssaoTexture;
[[vk::binding(12)]] SamplerCube<float4>   probeMap;
[[vk::binding(14)]] Sampler2D<float4>     diffuseTransmissionMap;

// KHR_texture_transform: t.xy = offset, t.zw = scale (no rotation support)
float2 applyUVTransform(float2 uv, float4 t) {
    return uv * t.zw + t.xy;
}

// Specular anti-aliasing (Tokuyoshi and Kaplanyan 2019): widen the lobe by
// the variance of the normal across the pixel. Without it a point light's
// GGX peak at low roughness is far above 1 and lands on a single pixel, a
// speck that bloom spreads and TAA cannot remove. A flat surface has no
// normal variance, so it gets no widening from this.
float widenRoughness(float roughness, float3 n) {
    static const float saaSigma2 = 0.25;
    static const float saaKappa  = 0.18;
    float3             dNdx      = ddx(n);
    float3             dNdy      = ddy(n);
    float              nVariance = saaSigma2 * (dot(dNdx, dNdx) + dot(dNdy, dNdy));
    float              a2        = pow(roughness, 4.0) + min(2.0 * nVariance, saaKappa);
    return sqrt(sqrt(min(a2, 1.0)));
}

// clang-format off
[shader("fragment")]
float4 fragMain(VSOutput input, float4 fragCoord : SV_Position) : SV_Target {
    // clang-format on
    float2 albedoUV = applyUVTransform(input.texCoords, params.albedoUVTransform);
    float3 texColor = pow(texture_diffuse1.Sample(albedoUV).rgb, float3(2.2));
    float3 albedo   = lerp(texColor, float3(1.0), float(params.hasNoTexture)) * params.baseColorFactor.rgb;

    float3 N = normalize(input.worldNormal);
    // The coat has no normal map of its own here, so it keeps the surface
    // normal and does not follow the base layer's.
    float3 Nc = N;
    // Gram-Schmidt leaves nothing to normalize where a mesh's tangent is
    // parallel to its normal (or zero), and normalize(0) is NaN, which bloom
    // then spreads into a dark blob. Keep the geometric normal there.
    float3 tangentOrtho = input.worldTangent.xyz - N * dot(N, input.worldTangent.xyz);
    if (params.hasNormalMap && dot(tangentOrtho, tangentOrtho) > 1e-12) {
        float2 normalUV = applyUVTransform(input.texCoords, params.normalUVTransform);
        float3 T        = normalize(tangentOrtho);
        float3 B        = cross(N, T) * input.worldTangent.w;
        // Z is reconstructed rather than sampled: baked normal maps are BC5,
        // which stores XY only. A tangent-space normal always points out of
        // the surface, so the positive root is the right one. Three-channel
        // normal maps give the same result, since their stored Z agrees with
        // their XY.
        float2 sampledXY = normalMap.Sample(normalUV).rg * 2.0 - 1.0;
        float  sampledZ  = sqrt(saturate(1.0 - dot(sampledXY, sampledXY)));
        N                = normalize(T * sampledXY.x + B * sampledXY.y + N * sampledZ);
    }

    // Per-pixel occlusion only (AO map x SSAO), kept apart from params.ao's
    // flat multiplier: the probe term below wants real geometric occlusion,
    // not the scene's flat ambient darkening.
    float aoTex = 1.0;
    if (params.hasAOMap) {
        float2 occlusionUV = applyUVTransform(input.texCoords, params.occlusionUVTransform);
        aoTex *= occlusionMap.Sample(occlusionUV).r;
    }
    if (params.ssaoEnabled) {
        aoTex *= ssaoTexture.Sample(fragCoord.xy / params.screenSize).r;
    }
    float ao = params.ao * aoTex;

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

    roughness = widenRoughness(roughness, N);

    // A coat is usually near mirror smooth, the case the widening exists for.
    float coatRoughness = widenRoughness(clamp(params.clearcoatRoughnessFactor, 0.089, 1.0), Nc);

    float3 transmissionTint = params.diffuseTransmission.rgb;
    float  transmission     = params.diffuseTransmission.w;
    if (params.hasDiffuseTransmissionMap) {
        // KHR_materials_diffuse_transmission: the strength is in alpha.
        float2 transmissionUV = applyUVTransform(input.texCoords, params.diffuseTransmissionUVTransform);
        transmission *= diffuseTransmissionMap.Sample(transmissionUV).a;
    }

    float3 V  = normalize(params.viewPos - input.worldPosition);
    float3 Lo = float3(0.0);

    // Directional light
    float3 dirContrib = evaluatePBR(params.directionalLight, input.worldPosition, albedo, N, V, metallic, roughness,
                                    Nc, params.clearcoatFactor, coatRoughness, transmission);
    float  shadow     = 0.0;
    if (params.directionalLight.castShadow) {
        shadow = shadowCascaded(shadowMap, params.lightSpaceMatrices, params.cascadeCount,
                                input.worldPosition, params.evsmBleedThreshold);
    }
    Lo += dirContrib * (1.0 - shadow) * float(params.directionalLight.enabled);
    if (transmission > 0.0) {
        // Unshadowed: the surface would shadow its own back light. Objects
        // between the light and the surface do not block it.
        Lo += evaluateTransmission(params.directionalLight, input.worldPosition, albedo, N,
                                   transmissionTint, transmission) *
              float(params.directionalLight.enabled);
    }

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
                              metallic, roughness,
                              Nc, params.clearcoatFactor, coatRoughness, transmission);
            if (transmission > 0.0) {
                Lo += evaluateTransmission(pl, input.worldPosition, albedo, N,
                                           transmissionTint, transmission);
            }
        }
    }

    float3 ambient = float3(params.ambientStrength) * albedo * ao;
    if (params.probeEnabled) {
        // Split-sum image-based specular from the probe. Diffuse ambient
        // keeps the flat term, weighted for the light the surface reflects
        // as specular and removed for metals.
        float  NdotV = max(dot(N, V), 1e-5);
        float3 F0    = lerp(float3(0.04), albedo, metallic);
        float3 F     = fresnelSchlickRoughness(NdotV, F0, roughness);
        float2 AB    = envBRDFApprox(NdotV, roughness);
        float3 R     = boxProject(reflect(-V, N), input.worldPosition,
                                  params.probePosition, params.probeBoxMin,
                                  params.probeBoxMax);
        float3 probe = probeMap.SampleLevel(R, roughness * params.probeMaxMip).rgb;
        ambient      = ambient * (1.0 - F) * (1.0 - metallic) +
                       probe * (F0 * AB.x + AB.y) * aoTex;
    }
    if (params.clearcoatFactor > 0.0) {
        // The coat takes its share of the ambient light off the base, and
        // reflects the probe on its own. Most of what a viewer sees of a coat
        // is this reflection, since a point light only gives a small highlight.
        float NdotVc = max(dot(Nc, V), 1e-5);
        float Fc     = params.clearcoatFactor * fresnelSchlickRoughness(NdotVc, float3(0.04), coatRoughness).x;
        ambient *= 1.0 - Fc;
        if (params.probeEnabled) {
            float2 AB    = envBRDFApprox(NdotVc, coatRoughness);
            float3 Rc    = boxProject(reflect(-V, Nc), input.worldPosition,
                                      params.probePosition, params.probeBoxMin,
                                      params.probeBoxMax);
            float3 probe = probeMap.SampleLevel(Rc, coatRoughness * params.probeMaxMip).rgb;
            ambient += probe * (0.04 * AB.x + AB.y) * params.clearcoatFactor * aoTex;
        }
    }
    float3 color = ambient + Lo + params.emissive + emissiveTexColor;

    // Linear HDR out. Tone mapping, gamma and dither happen once in
    // tonemap.slang, after bloom has extracted from this unbounded radiance.
    return float4(color, 1.0);
};
