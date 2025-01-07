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

// Missing Deadlines (Benjamin Wrensch): https://iolite-engine.com/blog_posts/minimal_agx_implementation
// Filament: https://github.com/google/filament/blob/main/filament/src/ToneMapper.cpp#L263
// https://github.com/EaryChow/AgX_LUT_Gen/blob/main/AgXBaseRec2020.py

// Three.js: https://github.com/mrdoob/three.js/blob/4993e3af579a27cec950401b523b6e796eab93ec/src/renderers/shaders/ShaderChunk/tonemapping_pars_fragment.glsl.js#L79-L89
// Matrices for rec 2020 <> rec 709 color space conversion
// matrix provided in row-major order so it has been transposed
// https://www.itu.int/pub/R-REP-BT.2407-2017
const mat3 LINEAR_REC2020_TO_LINEAR_SRGB = mat3(
  1.6605, -0.1246, -0.0182,
  -0.5876, 1.1329, -0.1006,
  -0.0728, -0.0083, 1.1187
);

const mat3 LINEAR_SRGB_TO_LINEAR_REC2020 = mat3(
  0.6274, 0.0691, 0.0164,
  0.3293, 0.9195, 0.0880,
  0.0433, 0.0113, 0.8956
);

// Converted to column major from blender: https://github.com/blender/blender/blob/fc08f7491e7eba994d86b610e5ec757f9c62ac81/release/datafiles/colormanagement/config.ocio#L358
const mat3 agx_mat = mat3(
  0.856627153315983, 0.137318972929847, 0.11189821299995,
  0.0951212405381588, 0.761241990602591, 0.0767994186031903,
  0.0482516061458583, 0.101439036467562, 0.811302368396859);

const float AgxMinEv = -12.47393f;
const float AgxMaxEv = 4.026069f;

vec3 agxDefaultContrastApprox(vec3 x) {
  vec3 x2 = x * x;
  vec3 x4 = x2 * x2;

  return + 15.5     * x4 * x2
         - 40.14    * x4 * x
         + 31.96    * x4
         - 6.868    * x2 * x
         + 0.4298   * x2
         + 0.1191   * x
         - 0.00232;
}

vec3 agx(vec3 val) {
  // From three.js
  val = LINEAR_SRGB_TO_LINEAR_REC2020 * val;

  // Input transform
  val = agx_mat * val;

  // From Filament: avoid 0 or negative numbers for log2
  val = max(val, 1e-10);

  // Log2 space encoding
  val = clamp(log2(val), AgxMinEv, AgxMaxEv);
  val = (val - AgxMinEv) / (AgxMaxEv - AgxMinEv);

  // From Filament
  val = clamp(val, 0.0, 1.0);

  // Apply sigmoid function approximation
  // Mean error^2: 3.6705141e-06
  val = agxDefaultContrastApprox(val);

  // sRGB IEC 61966-2-1 2.2 Exponent Reference EOTF Display
  // NOTE: We're linearizing the output here. Comment/adjust when
  // *not* using a sRGB render target
  // val = pow(max(vec3(0.0), val), vec3(2.2)); // From filament: max()

  // From three.js
  val = LINEAR_REC2020_TO_LINEAR_SRGB * val;

  // Gamut mapping
  val = clamp(val, 0.0, 1.0);

  return val;
}

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

    // AgX tonemapping
    color = agx(color);

    // gamma
    float gamma = 2.2;
    color = pow(max(vec3(0.0), color), vec3(1.0 / gamma));

    FragColor = vec4(color, 1.0);
}
