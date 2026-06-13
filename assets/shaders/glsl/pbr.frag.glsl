#version 330 core

layout(location = 0) out vec4 FragColor;

in VS_OUT {
    vec3 position;
    vec3 normal;
    vec2 texCoords;
    vec4 fragPosLightSpace;
}
fs_in;

// material parameters
uniform float metallic;
uniform float roughness;
uniform float ao;

// lights
struct DirectionalLight {
    bool  enabled;
    bool  castShadow;
    vec3  color;
    vec3  direction;
    float shadowBias;
};

struct PointLight {
    vec3 color;
    vec3 position;
    int  attenuationIndex;
};

uniform DirectionalLight directionalLight;
uniform int              numLights = 1;
uniform PointLight       pointLights[6];

uniform sampler2D texture_diffuse1;
uniform sampler2D shadowMap;
uniform int   shadowMode;         // 0=PCF 1=PCSS 2=DPCF 3=EVSM
uniform float pcssLightSize;      // light size for PCSS blocker search
uniform float pcfRadius;          // disk radius for DPCF
uniform float evsmBleedThreshold; // light bleed clamp for EVSM
uniform vec3      viewPos;
uniform float     ambientStrength;
uniform bool      hasNoTexture;

const float M_PI = 3.14159265359;

// Attenuation intensity; see https://learnopengl.com/Lighting/Light-casters
vec3 lightAttenuationData[11] =
    vec3[11](vec3(1.0, 0.7, 1.8), vec3(1.0, 0.35, 0.44), vec3(1.0, 0.22, 0.2),
             vec3(1.0, 0.14, 0.07), vec3(1.0, 0.09, 0.032),
             vec3(1.0, 0.07, 0.017), vec3(1.0, 0.045, 0.0075),
             vec3(1.0, 0.027, 0.0028), vec3(1.0, 0.022, 0.0019),
             vec3(1.0, 0.014, 0.0007), vec3(1.0, 0.007, 0.0002));

// function prototypes
float attenuationFromLight(PointLight light);
vec3  calculatePBR(vec3 albedo, vec3 N, vec3 V, vec3 L, vec3 radiance);
float shadowPCF(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                float shadowBias);
float shadowPCSS(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                 float shadowBias);
float shadowDPCF(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                 float shadowBias);
float shadowEVSM(vec4 fragPosLightSpace);
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                      float shadowBias);

// PBR function prototypes
vec3  fresnelSchlick(float cosTheta, vec3 F0);
float distributionGGX(vec3 N, vec3 H, float rough);
float geometrySchlickGGX(float NdotV, float rough);
float geometrySmith(float NdotV, float NdotL, float rough);

void main() {
    vec3 texColor =
        pow(texture(texture_diffuse1, fs_in.texCoords).rgb, vec3(2.2));
    vec3 constColor = pow(vec3(1.0), vec3(2.2));
    vec3 albedo     = mix(texColor, constColor, float(hasNoTexture));

    vec3 N = normalize(fs_in.normal);
    vec3 V = normalize(viewPos - fs_in.position);

    vec3 Lo = vec3(0.0);

    vec3 dirLightDir = normalize(-directionalLight.direction);
    vec3 dirRadiance = directionalLight.color;
    vec3 dirL        = calculatePBR(albedo, N, V, dirLightDir, dirRadiance);

    float shadow = 0.0;
    if (directionalLight.castShadow) {
        shadow = calculateShadow(fs_in.fragPosLightSpace, N, dirLightDir,
                                 directionalLight.shadowBias);
    }

    // zeroes out directional light if not enabled
    Lo += dirL * (1.0 - shadow) * float(directionalLight.enabled);

    for (int i = 0; i < numLights; i++) {
        PointLight light = pointLights[i];

        float attenuation = attenuationFromLight(light);
        vec3  radiance    = light.color * attenuation;
        vec3  lightDir    = normalize(light.position - fs_in.position);
        vec3  L           = calculatePBR(albedo, N, V, lightDir, radiance);

        Lo += L;
    }

    vec3 ambient = vec3(ambientStrength) * albedo * ao;

    vec3 color = ambient + Lo;

    // Reinhard HDR tone mapping
    color = color / (color + vec3(1.0));

    float gamma = 2.2;
    color       = pow(color, vec3(1.0 / gamma));

    FragColor = vec4(color, 1.0);
}

float attenuationFromLight(PointLight light) {
    float distance    = length(light.position - fs_in.position);
    int   index       = clamp(light.attenuationIndex, 0, 10);
    vec3  attenuation = lightAttenuationData[index];
    return 1.0 / (attenuation.x + attenuation.y * distance +
                  attenuation.z * (distance * distance));
}

vec3 calculatePBR(vec3 albedo, vec3 N, vec3 V, vec3 L, vec3 radiance) {
    vec3 result = vec3(0.0);

    // calculate reflectance at normal incidence; if dia-electric (like plastic)
    // use F0 of 0.04 and if it's a metal, use the albedo color as F0 (metallic
    // workflow)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // light contribution
    vec3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 1e-5);
    float NdotL = max(dot(N, L), 1e-5);

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G   = geometrySmith(NdotV, NdotL, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  numerator   = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 1e-5;
    vec3  specular    = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= vec3(1.0 - metallic);

    result += (kD * albedo / M_PI + specular) * radiance * NdotL;

    return result;
}

// Poisson disk for DPCF (16 samples)
const vec2 poissonDisk[16] = vec2[16](
    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790));

float shadowPCF(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                float shadowBias) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords      = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.z < 0.0) {
        return 0.0;
    }
    float bias         = max(shadowBias * (1.0 - dot(normal, lightDir)), 0.005);
    float currentDepth = projCoords.z - bias;
    float shadow       = 0.0;
    vec2  texelSize    = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float d =
                texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth > d ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

float findBlockerDistance(vec2 uv, float currentDepth, float searchWidth) {
    float blockerSum   = 0.0;
    int   blockerCount = 0;
    vec2  texelSize    = 1.0 / textureSize(shadowMap, 0);
    for (int x = -2; x <= 1; ++x) {
        for (int y = -2; y <= 1; ++y) {
            float d =
                texture(shadowMap, uv + vec2(x, y) * texelSize * searchWidth).r;
            if (d < currentDepth) {
                blockerSum += d;
                ++blockerCount;
            }
        }
    }
    return blockerCount == 0 ? -1.0 : blockerSum / float(blockerCount);
}

float shadowPCSS(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                 float shadowBias) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords      = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.z < 0.0) {
        return 0.0;
    }
    float bias         = max(shadowBias * (1.0 - dot(normal, lightDir)), 0.005);
    float currentDepth = projCoords.z - bias;

    float avgBlocker =
        findBlockerDistance(projCoords.xy, currentDepth, pcssLightSize * 10.0);
    if (avgBlocker < 0.0) {
        return 0.0;
    }

    float penumbra  = (currentDepth - avgBlocker) / avgBlocker * pcssLightSize;
    float shadow    = 0.0;
    vec2  texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float d = texture(shadowMap,
                              projCoords.xy +
                                  vec2(x, y) * texelSize * penumbra * 10.0)
                          .r;
            shadow += currentDepth > d ? 1.0 : 0.0;
        }
    }
    return shadow / 25.0;
}

float shadowDPCF(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                 float shadowBias) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords      = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.z < 0.0) {
        return 0.0;
    }
    float bias         = max(shadowBias * (1.0 - dot(normal, lightDir)), 0.005);
    float currentDepth = projCoords.z - bias;
    float shadow       = 0.0;
    vec2  texelSize    = 1.0 / textureSize(shadowMap, 0);
    for (int i = 0; i < 16; ++i) {
        float d = texture(shadowMap,
                          projCoords.xy + poissonDisk[i] * texelSize * pcfRadius)
                      .r;
        shadow += currentDepth > d ? 1.0 : 0.0;
    }
    return shadow / 16.0;
}

float shadowEVSM(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords      = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.z < 0.0) {
        return 0.0;
    }
    float currentDepth = projCoords.z;
    vec2  moments      = texture(shadowMap, projCoords.xy).rg;

    float p        = float(currentDepth <= moments.x);
    float variance = moments.y - moments.x * moments.x;
    variance       = max(variance, 0.00002);
    float d        = currentDepth - moments.x;
    float pMax     = variance / (variance + d * d);
    pMax = clamp((pMax - evsmBleedThreshold) / (1.0 - evsmBleedThreshold), 0.0,
                 1.0);
    return 1.0 - max(p, pMax);
}

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir,
                      float shadowBias) {
    if (shadowMode == 1) {
        return shadowPCSS(fragPosLightSpace, normal, lightDir, shadowBias);
    }
    if (shadowMode == 2) {
        return shadowDPCF(fragPosLightSpace, normal, lightDir, shadowBias);
    }
    if (shadowMode == 3) {
        return shadowEVSM(fragPosLightSpace);
    }
    return shadowPCF(fragPosLightSpace, normal, lightDir, shadowBias);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    // Optimized exponent version, from:
    // http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
    return F0 +
           (1.0 - F0) * pow(2.0, (-5.55473 * cosTheta - 6.98316) * cosTheta);
}

float distributionGGX(vec3 N, vec3 H, float rough) {
    float a  = rough * rough;
    float a2 = a * a;

    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom       = M_PI * denom * denom;

    return nom / denom;
}

float geometrySchlickGGX(float NdotV, float rough) {
    float r = (rough + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometrySmith(float NdotV, float NdotL, float rough) {
    float ggx2 = geometrySchlickGGX(NdotV, rough);
    float ggx1 = geometrySchlickGGX(NdotL, rough);

    return ggx1 * ggx2;
}
