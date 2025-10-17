#version 330 core

layout (location = 0) out vec4 FragColor;

in vec3 vPosition;
in vec2 vTexCoord;
in vec3 vNormal;
in vec4 vFragPosLightSpace;

// material parameters
uniform float metallic;
uniform float roughness;
uniform float ao;

// lights
struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};

uniform int numLights = 1;
uniform PointLight pointLights[6];

uniform sampler2D texture_diffuse1;
uniform sampler2D shadowMap;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform bool hasNoTexture;

const float M_PI = 3.14159265359;

// function prototypes
float attenuationFromLight(PointLight light);
vec3 calculatePBR(vec3 albedo, vec3 N, vec3 V, vec3 L, vec3 radiance);
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);

// PBR function prototypes
vec3 fresnelSchlick(float cosTheta, vec3 F0);
float distributionGGX(vec3 N, vec3 H, float rough);
float geometrySchlickGGX(float NdotV, float rough);
float geometrySmith(float NdotV, float NdotL, float rough);

void main() {
    vec3 texColor = pow(texture(texture_diffuse1, vTexCoord).rgb, vec3(2.2));
    vec3 constColor = pow(vec3(1.0), vec3(2.2));
    vec3 albedo = mix(texColor, constColor, float(hasNoTexture));

    vec3 N = normalize(vNormal);
    vec3 V = normalize(viewPos - vPosition);

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < numLights; i++) {
        PointLight light = pointLights[i];

        float attenuation = attenuationFromLight(light);
        vec3 radiance = light.color * attenuation;
        vec3 lightDir = normalize(light.position - vPosition);
        vec3 L = calculatePBR(albedo, N, V, lightDir, radiance);

        // apply shadow
        float shadow = calculateShadow(vFragPosLightSpace, N, lightDir);
        Lo += L * (1.0 - shadow);
    }

    vec3 ambient = vec3(ambientStrength) * albedo * ao;

    vec3 color = ambient + Lo;

    // Reinhard HDR tonemapping
    color = color / (color + vec3(1.0));

    float gamma = 2.2;
    color = pow(color, vec3(1.0 / gamma));

    FragColor = vec4(color, 1.0);
}

float attenuationFromLight(PointLight light) {
    float distance = length(light.position - vPosition);
    return 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
}

vec3 calculatePBR(vec3 albedo, vec3 N, vec3 V, vec3 L, vec3 radiance) {
    vec3 result = vec3(0.0);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // light contribution
    vec3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(NdotV, NdotL, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 1e-5f;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= vec3(1.0 - metallic);

    result += (kD * albedo / M_PI + specular) * radiance * NdotL;

    return result;
}

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get closest depth value from light's perspective
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // get current depth value
    float currentDepth = projCoords.z;

    // calculate bias based on light direction and normal (helps with peter panning and shadow acne)
    float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.01);

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; y++)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;


    // keep shadow at 0.0 when outside the far plane region of the light's frustum
    if (projCoords.z > 1.0 || projCoords.z < 0.0) {
        shadow = 0.0;
    }

    return shadow;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    // Optimized exponent version, from: http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * cosTheta - 6.98316) * cosTheta);
}

float distributionGGX(vec3 N, vec3 H, float rough) {
    float a = rough * rough;
    float a2 = a * a;

    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = M_PI * denom * denom;

    return nom / denom;
}

float geometrySchlickGGX(float NdotV, float rough) {
    float r = (rough + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometrySmith(float NdotV, float NdotL, float rough) {
    float ggx2 = geometrySchlickGGX(NdotV, rough);
    float ggx1 = geometrySchlickGGX(NdotL, rough);

    return ggx1 * ggx2;
}
