// probe_prefilter.slang: convolves the probe capture with GGX for one
// roughness level of one cube face (Karis 2013, "Real Shading in Unreal
// Engine 4"). Runs once at load, drawn with screenquad.vert into each face
// and mip of the result cubemap. N = V = R, the usual split-sum assumption.
#include "include/lighting.slang"

struct FSInput {
    float2 texCoords : TEXCOORD0;
};

[[vk::binding(0)]] SamplerCube<float4> environment;

uniform int   face;
uniform float roughness;
uniform float faceSize;

static const uint sampleCount = 512;

// Direction through texel (s, t) of a GL cube face. t = 0 is the first row
// of the face, which is the bottom row drawn into the framebuffer.
float3 faceDirection(int f, float2 st) {
    float2 uv = st * 2.0 - 1.0;
    switch (f) {
        case 0:
            return float3(1.0, -uv.y, -uv.x);
        case 1:
            return float3(-1.0, -uv.y, uv.x);
        case 2:
            return float3(uv.x, 1.0, uv.y);
        case 3:
            return float3(uv.x, -1.0, -uv.y);
        case 4:
            return float3(uv.x, -uv.y, 1.0);
        default:
            return float3(-uv.x, -uv.y, -1.0);
    }
}

float2 hammersley(uint i, uint n) {
    return float2(float(i) / float(n), float(reversebits(i)) * 2.3283064365386963e-10);
}

float3 importanceSampleGGX(float2 xi, float3 N, float rough) {
    float  a        = rough * rough;
    float  phi      = 2.0 * M_PI * xi.x;
    float  cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float  sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float3 H        = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    float3 up       = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent  = normalize(cross(up, N));
    float3 bitan    = cross(N, tangent);
    return normalize(tangent * H.x + bitan * H.y + N * H.z);
}

// clang-format off
[shader("fragment")]
float4 fragMain(FSInput input) : SV_Target {
    // clang-format on
    float3 N = normalize(faceDirection(face, input.texCoords));

    // Solid angle of one level-0 texel, to pick the source mip whose texels
    // match each sample's footprint.
    float  saTexel = 4.0 * M_PI / (6.0 * faceSize * faceSize);
    float3 color   = float3(0.0);
    float  weight  = 0.0;
    for (uint i = 0; i < sampleCount; i++) {
        float3 H     = importanceSampleGGX(hammersley(i, sampleCount), N, roughness);
        float3 L     = normalize(2.0 * dot(N, H) * H - N);
        float  NdotL = dot(N, L);
        if (NdotL > 0.0) {
            float pdf      = distributionGGX(N, H, roughness) * 0.25 + 1e-4;
            float saSample = 1.0 / (float(sampleCount) * pdf + 1e-4);
            float mip      = 0.5 * log2(saSample / saTexel);
            color += environment.SampleLevel(L, max(mip, 0.0)).rgb * NdotL;
            weight += NdotL;
        }
    }
    return float4(color / max(weight, 1e-4), 1.0);
}
