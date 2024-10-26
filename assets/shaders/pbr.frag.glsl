#version 330 core

layout (location = 0) out vec4 FragColor;

in vec3 gPosition;
in vec2 gTexCoord;
in vec3 gNormal;
in vec3 gColor;
noperspective in vec3 gEdgeDistance;

// material parameters
uniform float metallic;
uniform float roughness;
uniform float ao;

// lights
uniform vec3 lightPos;
uniform vec3 lightColor;

uniform sampler2D texture_diffuse1;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform bool hasNoTexture;
uniform bool showWireframe;
uniform vec3 lineColor;
uniform float lineWidth;

const float M_PI = 3.14159265359;

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = M_PI * denom * denom;

    return nom / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
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

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 albedo;

    if (hasNoTexture) {
        albedo = pow(gColor, vec3(2.2));
    } else {
        albedo = texture(texture_diffuse1, gTexCoord).rgb;
    }

    vec3 N = normalize(gNormal);
    vec3 V = normalize(viewPos - gPosition);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);

    // calculate per-light radiance
    vec3 L = normalize(lightPos - gPosition);
    vec3 H = normalize(V + L);
    float distance = length(lightPos - gPosition);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lightColor * attenuation;

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    // kS is equal to Fresnel
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= vec3(1.0 - metallic);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / M_PI + specular) * radiance * NdotL;

    // ambient
    vec3 ambient = vec3(ambientStrength) * albedo * ao;
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));

    // gamma
    float gamma = 2.2;
    color = pow(color, vec3(1.0 / gamma));

    float mixVal = 1.0;
    if (showWireframe) {
        float d = min(gEdgeDistance.x, gEdgeDistance.y);
        d = min(d, gEdgeDistance.z);
        mixVal = smoothstep(lineWidth - 1, lineWidth + 1, d);
    }

    FragColor = mix(vec4(lineColor, 1.0), vec4(color, 1.0), mixVal);
}