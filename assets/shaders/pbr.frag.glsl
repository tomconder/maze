#version 330 core

layout (location = 0) out vec4 FragColor;

in vec3 vPosition;
in vec2 vTexCoord;
in vec3 vNormal;

// material parameters
uniform float metallic;
uniform float roughness;
uniform float ao;

// lights
struct PointLight {
    vec3 position;
    vec3 color;
    vec3 attenuation;
};

uniform int numLights = 1;
uniform PointLight pointLights[6];

uniform sampler2D texture_diffuse1;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform bool hasNoTexture;

const float M_PI = 3.14159265359;

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = M_PI * denom * denom;

    return nom / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float attenuationFromLight(PointLight light) {
    float distance = length(light.position - vPosition);
    float constant = light.attenuation.r;
    float linear = light.attenuation.g;
    float quadratic = light.attenuation.b;
    return 1.0 / (constant + linear * distance + quadratic * (distance * distance));
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    // Optimized exponent version, from: http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * cosTheta - 6.98316) * cosTheta);
}

void main() {
    vec3 albedo;

    if (hasNoTexture) {
        albedo = pow(vec3(1.0), vec3(2.2));
    } else {
        albedo = pow(texture(texture_diffuse1, vTexCoord).rgb, vec3(2.2));
    }

    vec3 N = normalize(vNormal);
    vec3 V = normalize(viewPos - vPosition);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < numLights; i++) {
        // calculate per-light radiance
        vec3 L = normalize(pointLights[i].position - vPosition);
        vec3 H = normalize(V + L);
        float attenuation = attenuationFromLight(pointLights[i]);
        vec3 radiance = pointLights[i].color * attenuation;

        // Cook-Torrance BRDF
        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= vec3(1.0 - metallic);

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * albedo / M_PI + specular) * radiance * NdotL;
    }

    // ambient
    vec3 ambient = vec3(ambientStrength) * albedo * ao;

    vec3 color = ambient + Lo;

    // Reinhard HDR tonemapping
    color = color / (color + vec3(1.0));

    // gamma
    float gamma = 2.2;
    color = pow(color, vec3(1.0 / gamma));

    FragColor = vec4(color, 1.0);
}
