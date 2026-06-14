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
    bool enabled;
    bool castShadow;
    vec3 color;
    vec3 direction;
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
uniform float evsmBleedThreshold;
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
float shadowEVSM(vec4 fragPosLightSpace);

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
        shadow = shadowEVSM(fs_in.fragPosLightSpace);
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
