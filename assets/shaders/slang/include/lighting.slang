#pragma once

static const float M_PI = 3.14159265359;

// Attenuation coefficients [constant, linear, quadratic] per distance range.
// See https://learnopengl.com/Lighting/Light-casters
static const float3 lightAttenuation[11] = {
    float3(1.0, 0.70, 1.800), float3(1.0, 0.35, 0.440),
    float3(1.0, 0.22, 0.200), float3(1.0, 0.14, 0.070),
    float3(1.0, 0.09, 0.032), float3(1.0, 0.07, 0.017),
    float3(1.0, 0.045, 0.0075), float3(1.0, 0.027, 0.0028),
    float3(1.0, 0.022, 0.0019), float3(1.0, 0.014, 0.0007),
    float3(1.0, 0.007, 0.0002)
};

// Culling radii (world units) per attenuation index. Radiance is windowed to
// zero at this radius so cluster culling produces no visible seams.
// Solved from 1/(1 + l*d + q*d^2) = 0.05 per index: cutting the <=5% tail
// (vs the ~1% learnopengl nominal ranges) halves the radii so far fewer
// lights overlap each cluster.
static const float attenuationRadius[11] = {
    3.0, 6.0, 9.0, 16.0, 23.0, 31.0,
    47.0, 78.0, 94.0, 155.0, 290.0
};

// ---------------------------------------------------------------------------
// Light interface.
// evaluatePBR<L:ILight>() is monomorphized per concrete type at compile time.
// ---------------------------------------------------------------------------
interface ILight {
    // World-space unit vector from worldPos toward the light.
    float3 getDirection(float3 worldPos);

    // Pre-attenuated radiance arriving at worldPos.
    float3 getRadiance(float3 worldPos);
}

// ---------------------------------------------------------------------------
// Concrete light types
// ---------------------------------------------------------------------------

struct DirectionalLight {
    bool   enabled;
    bool   castShadow;
    float3 color;
    float3 direction;
};

extension DirectionalLight : ILight {
    float3 getDirection(float3 worldPos) { return normalize(-direction); }
    float3 getRadiance(float3 worldPos) { return color; }
}

struct PointLight {
    float3 color;
    float3 position;
    int    attenuationIndex;
};

extension PointLight : ILight {
    float3 getDirection(float3 worldPos) { return normalize(position - worldPos); }
    float3 getRadiance(float3 worldPos) {
        float  dist  = length(position - worldPos);
        int    idx   = clamp(attenuationIndex, 0, 10);
        float3 att   = lightAttenuation[idx];
        float  atten = 1.0 / (att.x + att.y * dist + att.z * dist * dist);
        // Window to zero at the culling radius (Karis falloff) so lights
        // culled per-cluster never leave a hard seam at cluster boundaries.
        float dr     = dist / attenuationRadius[idx];
        float dr2    = dr * dr;
        float window = saturate(1.0 - dr2 * dr2);
        return color * atten * window * window;
    }
}

// ---------------------------------------------------------------------------
// PBR BRDF helpers
// ---------------------------------------------------------------------------

float3 fresnelSchlick(float cosTheta, float3 F0) {
    // Optimized exponent (Karis 2013)
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * cosTheta - 6.98316) * cosTheta);
}

// Fresnel for ambient light, which arrives from every direction: rough
// surfaces get less edge brightening (Lagarde). Rounding can put a dot
// product of unit vectors just above 1, and pow() of a negative base is NaN,
// so the base is clamped.
float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float rough) {
    return F0 + (max(float3(1.0 - rough), F0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// Scale and bias for F0 in the split-sum environment BRDF, fitted
// analytically (Karis, "Physically Based Shading on Mobile", 2014), so no
// lookup texture is needed. Returns (A, B) for F0 * A + B.
float2 envBRDFApprox(float NdotV, float rough) {
    const float4 c0   = float4(-1.0, -0.0275, -0.572, 0.022);
    const float4 c1   = float4(1.0, 0.0425, 1.04, -0.04);
    float4       r    = rough * c0 + c1;
    float        a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    return float2(-1.04, 1.04) * a004 + r.zw;
}

// Box projection: the direction from the probe center to where the ray from
// P along R leaves the box, so a probe lookup lands on the right wall.
float3 boxProject(float3 R, float3 P, float3 center, float3 boxMin, float3 boxMax) {
    float3 tFar = max((boxMax - P) / R, (boxMin - P) / R);
    float  t    = min(min(tFar.x, tFar.y), tFar.z);
    // P is outside the box: t < 0 would point the lookup behind the probe.
    if (t < 0.0) {
        return R;
    }
    return P + R * t - center;
}

// Where the view points exactly against the light, V + L is zero and
// normalize() gives NaN. H is left zero there instead. The light then adds
// almost nothing, since N.V and N.L can't both be positive.
float3 halfVector(float3 V, float3 L) {
    float3 h = V + L;
    return h * rsqrt(max(dot(h, h), 1e-12));
}

float distributionGGX(float3 N, float3 H, float rough) {
    float a     = rough * rough;
    float a2    = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (M_PI * denom * denom);
}

float geometrySchlickGGX(float NdotV, float rough) {
    float r = rough + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(float NdotV, float NdotL, float rough) {
    return geometrySchlickGGX(NdotV, rough) * geometrySchlickGGX(NdotL, rough);
}

// ---------------------------------------------------------------------------
// Generic PBR evaluation, resolved at compile time per light type.
// ---------------------------------------------------------------------------
float3 evaluatePBR<L : ILight>(
    L      light,
    float3 worldPos,
    float3 albedo,
    float3 N,
    float3 V,
    float  metallic,
    float  roughness,
    float3 Nc,  // coat normal: the geometric normal, no normal map
    float  clearcoat,
    float  coatRoughness) {
    float3 Ldir     = light.getDirection(worldPos);
    float3 radiance = light.getRadiance(worldPos);
    float3 H        = halfVector(V, Ldir);
    float  NdotV    = max(dot(N, V), 1e-5);
    float  NdotL    = max(dot(N, Ldir), 1e-5);
    float3 F0       = lerp(float3(0.04), albedo, metallic);
    float  NDF      = distributionGGX(N, H, roughness);
    float  G        = geometrySmith(NdotV, NdotL, roughness);
    float3 F        = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float3 specular = (NDF * G * F) / (4.0 * NdotV * NdotL + 1e-5);
    float3 kD       = (float3(1.0) - F) * (1.0 - metallic);
    float3 base     = (kD * albedo / M_PI + specular) * radiance * NdotL;
    if (clearcoat <= 0.0) {
        return base;
    }

    // Clear coat: a second GGX lobe at a fixed F0 of 0.04 over the base. The
    // base loses what the coat reflects.
    float NdotVc = max(dot(Nc, V), 1e-5);
    float NdotLc = max(dot(Nc, Ldir), 1e-5);
    float Fc     = clearcoat * fresnelSchlick(max(dot(H, V), 0.0), float3(0.04)).x;
    float coat   = distributionGGX(Nc, H, coatRoughness) *
                   geometrySmith(NdotVc, NdotLc, coatRoughness) * Fc /
                   (4.0 * NdotVc * NdotLc + 1e-5);
    return base * (1.0 - Fc) + coat * radiance * NdotLc;
}

// Specular lobe only. Glass adds this to a refracted background, so the
// diffuse term in evaluatePBR would light the surface a second time.
float3 evaluateSpecular<L : ILight>(
    L      light,
    float3 worldPos,
    float3 N,
    float3 V,
    float3 F0,
    float  roughness) {
    float3 Ldir     = light.getDirection(worldPos);
    float3 radiance = light.getRadiance(worldPos);
    float3 H        = halfVector(V, Ldir);
    float  NdotV    = max(dot(N, V), 1e-5);
    float  NdotL    = max(dot(N, Ldir), 1e-5);
    float  NDF      = distributionGGX(N, H, roughness);
    float  G        = geometrySmith(NdotV, NdotL, roughness);
    float3 F        = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float3 specular = (NDF * G * F) / (4.0 * NdotV * NdotL + 1e-5);
    return specular * radiance * NdotL;
}
