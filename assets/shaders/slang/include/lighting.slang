#pragma once

static const float M_PI = 3.14159265359;

// Attenuation coefficients [constant, linear, quadratic] per distance range.
// See https://learnopengl.com/Lighting/Light-casters
static const float3 lightAttenuation[11] = {
    float3(1.0, 0.70,  1.800),  float3(1.0, 0.35,  0.440),
    float3(1.0, 0.22,  0.200),  float3(1.0, 0.14,  0.070),
    float3(1.0, 0.09,  0.032),  float3(1.0, 0.07,  0.017),
    float3(1.0, 0.045, 0.0075), float3(1.0, 0.027, 0.0028),
    float3(1.0, 0.022, 0.0019), float3(1.0, 0.014, 0.0007),
    float3(1.0, 0.007, 0.0002)
};

// Culling radii (world units) per attenuation index. Radiance is windowed to
// zero at this radius so cluster culling produces no visible seams.
static const float attenuationRadius[11] = {
    7.0,  13.0,  20.0,  32.0,  50.0,  65.0,
    100.0, 160.0, 200.0, 325.0, 600.0
};

// ---------------------------------------------------------------------------
// Light interface — all light types conform to this.
// evaluatePBR<L:ILight>() is monomorphized per concrete type at compile time.
// ---------------------------------------------------------------------------
interface ILight
{
    // World-space unit vector from worldPos toward the light.
    float3 getDirection(float3 worldPos);

    // Pre-attenuated radiance arriving at worldPos.
    float3 getRadiance(float3 worldPos);
}

// ---------------------------------------------------------------------------
// Concrete light types
// ---------------------------------------------------------------------------

struct DirectionalLight
{
    bool   enabled;
    bool   castShadow;
    float3 color;
    float3 direction;
}

extension DirectionalLight : ILight
{
    float3 getDirection(float3 worldPos) { return normalize(-direction); }
    float3 getRadiance(float3 worldPos)  { return color; }
}

struct PointLight
{
    float3 color;
    float3 position;
    int    attenuationIndex;
}

extension PointLight : ILight
{
    float3 getDirection(float3 worldPos) { return normalize(position - worldPos); }
    float3 getRadiance(float3 worldPos)
    {
        float  dist = length(position - worldPos);
        int    idx  = clamp(attenuationIndex, 0, 10);
        float3 att  = lightAttenuation[idx];
        float  atten = 1.0 / (att.x + att.y * dist + att.z * dist * dist);
        // Window to zero at the culling radius (Karis falloff) so lights
        // culled per-cluster never leave a hard seam at cluster boundaries.
        float  dr     = dist / attenuationRadius[idx];
        float  dr2    = dr * dr;
        float  window = saturate(1.0 - dr2 * dr2);
        return color * atten * window * window;
    }
}

// ---------------------------------------------------------------------------
// PBR BRDF helpers
// ---------------------------------------------------------------------------

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    // Optimized exponent — Karis 2013
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * cosTheta - 6.98316) * cosTheta);
}

float distributionGGX(float3 N, float3 H, float rough)
{
    float a      = rough * rough;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float denom  = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (M_PI * denom * denom);
}

float geometrySchlickGGX(float NdotV, float rough)
{
    float r = rough + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(float NdotV, float NdotL, float rough)
{
    return geometrySchlickGGX(NdotV, rough) * geometrySchlickGGX(NdotL, rough);
}

// ---------------------------------------------------------------------------
// Generic PBR evaluation — resolved at compile time per light type.
// ---------------------------------------------------------------------------
float3 evaluatePBR<L : ILight>(
    L      light,
    float3 worldPos,
    float3 albedo,
    float3 N,
    float3 V,
    float  metallic,
    float  roughness)
{
    float3 Ldir     = light.getDirection(worldPos);
    float3 radiance = light.getRadiance(worldPos);
    float3 H        = normalize(V + Ldir);
    float  NdotV    = max(dot(N, V),    1e-5);
    float  NdotL    = max(dot(N, Ldir), 1e-5);
    float3 F0       = lerp(float3(0.04), albedo, metallic);
    float  NDF      = distributionGGX(N, H, roughness);
    float  G        = geometrySmith(NdotV, NdotL, roughness);
    float3 F        = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float3 specular = (NDF * G * F) / (4.0 * NdotV * NdotL + 1e-5);
    float3 kD       = (float3(1.0) - F) * (1.0 - metallic);
    return (kD * albedo / M_PI + specular) * radiance * NdotL;
}
