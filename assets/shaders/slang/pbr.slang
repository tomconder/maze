// pbr.slang — PBR vertex + fragment shader
#include "include/uniforms.slang"
#include "include/shadows.slang"

// Slang does not expose inverse() for matrices; compute 3x3 inverse analytically.
float3x3 inverse3x3(float3x3 m)
{
    float3 c0 = float3(m[0][0], m[1][0], m[2][0]);
    float3 c1 = float3(m[0][1], m[1][1], m[2][1]);
    float3 c2 = float3(m[0][2], m[1][2], m[2][2]);
    float3 row0 = cross(c1, c2);
    float3 row1 = cross(c2, c0);
    float3 row2 = cross(c0, c1);
    float det = dot(c0, row0);
    return float3x3(row0 / det, row1 / det, row2 / det);
}

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal   : NORMAL;
}

struct Varyings
{
    float4 pos              : SV_Position;
    float3 worldPosition    : TEXCOORD0;
    float3 worldNormal      : TEXCOORD1;
    float2 texCoords        : TEXCOORD2;
    float4 fragPosLightSpace: TEXCOORD3;
}

[shader("vertex")]
Varyings vertMain(VSInput input)
{
    Varyings output;
    float4 worldPos4        = mul(params.model, float4(input.position, 1.0));
    output.pos              = mul(params.mvp,   float4(input.position, 1.0));
    output.worldPosition    = worldPos4.xyz;
    float3x3 normalMatrix   = transpose(inverse3x3((float3x3)params.model));
    output.worldNormal      = mul(normalMatrix, input.normal);
    output.texCoords        = input.texCoord;
    output.fragPosLightSpace= mul(params.lightSpaceMatrix, worldPos4);
    return output;
}

// Explicit binding pins the texture unit the C++ side binds to.
[[vk::binding(0)]] Sampler2D<float4> texture_diffuse1;
[[vk::binding(1)]] Sampler2D<float2> shadowMap;

[shader("fragment")]
float4 fragMain(Varyings input) : SV_Target
{
    float3 texColor   = pow(texture_diffuse1.Sample(input.texCoords).rgb, float3(2.2));
    float3 constColor = pow(float3(1.0), float3(2.2));
    float3 albedo     = lerp(texColor, constColor, float(params.hasNoTexture));

    float3 N  = normalize(input.worldNormal);
    float3 V  = normalize(params.viewPos - input.worldPosition);
    float3 Lo = float3(0.0);

    // Directional light
    float3 dirContrib = evaluatePBR(params.directionalLight, input.worldPosition, albedo, N, V, params.metallic, params.roughness);
    float  shadow     = 0.0;
    if (params.directionalLight.castShadow)
    {
        shadow = shadowEVSM(shadowMap, input.fragPosLightSpace, params.evsmBleedThreshold);
    }
    Lo += dirContrib * (1.0 - shadow) * float(params.directionalLight.enabled);

    // Point lights
    for (int i = 0; i < params.numLights; i++)
    {
        Lo += evaluatePBR(params.pointLights[i], input.worldPosition, albedo, N, V, params.metallic, params.roughness);
    }

    float3 ambient = float3(params.ambientStrength) * albedo * params.ao;
    float3 color   = ambient + Lo;

    // Reinhard HDR tone mapping
    color = color / (color + float3(1.0));

    // Gamma correction
    color = pow(color, float3(1.0 / 2.2));

    return float4(color + params.emissive, 1.0);
}
