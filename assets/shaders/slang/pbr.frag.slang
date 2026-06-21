// pbr.frag.slang
#include "include/lighting.slang"
#include "include/shadows.slang"

// Material uniforms
uniform float metallic;
uniform float roughness;
uniform float ao;
uniform float ambientStrength;
uniform float evsmBleedThreshold;
uniform bool  hasNoTexture;
uniform float3 viewPos;
uniform float3 emissive;

// Light uniforms (set member-by-member via glUniform* — matches existing C++)
uniform DirectionalLight directionalLight;
uniform int              numLights;
uniform PointLight       pointLights[6];

// Textures — HLSL register syntax; Slang combines Texture2D+SamplerState for GLSL target
Texture2D<float4> texture_diffuse1 : register(t0);
SamplerState      sampler_diffuse1 : register(s0);
Texture2D<float2> shadowMap        : register(t1);
SamplerState      sampler_shadowMap: register(s1);

struct FSInput
{
    float4 pos              : SV_Position;
    float3 worldPosition    : TEXCOORD0;
    float3 worldNormal      : TEXCOORD1;
    float2 texCoords        : TEXCOORD2;
    float4 fragPosLightSpace: TEXCOORD3;
}

[shader("fragment")]
float4 fragMain(FSInput input) : SV_Target
{
    float3 texColor   = pow(texture_diffuse1.Sample(sampler_diffuse1, input.texCoords).rgb, float3(2.2));
    float3 constColor = pow(float3(1.0), float3(2.2));
    float3 albedo     = lerp(texColor, constColor, float(hasNoTexture));

    float3 N  = normalize(input.worldNormal);
    float3 V  = normalize(viewPos - input.worldPosition);
    float3 Lo = float3(0.0);

    // Directional light — evaluatePBR<DirectionalLight> specialized at compile time
    float3 dirContrib = evaluatePBR(directionalLight, input.worldPosition, albedo, N, V, metallic, roughness);
    float  shadow     = 0.0;
    if (directionalLight.castShadow)
        shadow = shadowEVSM(shadowMap, sampler_shadowMap, input.fragPosLightSpace, evsmBleedThreshold);
    Lo += dirContrib * (1.0 - shadow) * float(directionalLight.enabled);

    // Point lights — evaluatePBR<PointLight> specialized at compile time
    for (int i = 0; i < numLights; ++i)
        Lo += evaluatePBR(pointLights[i], input.worldPosition, albedo, N, V, metallic, roughness);

    float3 ambient = float3(ambientStrength) * albedo * ao;
    float3 color   = ambient + Lo;

    // Reinhard HDR tone mapping
    color = color / (color + float3(1.0));

    // Gamma correction
    color = pow(color, float3(1.0 / 2.2));

    return float4(color + emissive, 1.0);
}
