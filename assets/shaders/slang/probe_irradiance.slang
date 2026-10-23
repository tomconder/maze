// probe_irradiance.slang: convolves the probe capture with a cosine lobe for
// one face of the irradiance cubemap. The result is the mean radiance over the
// hemisphere around N weighted by cos, which is E / pi, so a Lambert surface
// multiplies it by its albedo and nothing else. Runs once at load, drawn with
// screenquad.vert into each face of the irradiance cubemap.
#include "include/lighting.slang"
#include "include/probe.slang"

struct FSInput {
    float2 texCoords : TEXCOORD0;
};

[[vk::binding(0)]] SamplerCube<float4> environment;

uniform int   face;
uniform float faceSize;

static const uint sampleCount = 256;

// clang-format off
[shader("fragment")]
float4 fragMain(FSInput input) : SV_Target {
    // clang-format on
    float3 N       = normalize(faceDirection(face, input.texCoords));
    float3 up      = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, N));
    float3 bitan   = cross(N, tangent);

    // Solid angle of one level-0 texel, to pick the source mip whose texels
    // match each sample's footprint. This keeps 256 samples free of noise.
    float  saTexel = 4.0 * M_PI / (6.0 * faceSize * faceSize);
    float3 color   = float3(0.0);
    for (uint i = 0; i < sampleCount; i++) {
        float2 xi       = hammersley(i, sampleCount);
        float  phi      = 2.0 * M_PI * xi.x;
        float  cosTheta = sqrt(1.0 - xi.y);
        float  sinTheta = sqrt(xi.y);
        float3 L        = tangent * (cos(phi) * sinTheta) + bitan * (sin(phi) * sinTheta) + N * cosTheta;
        // Cosine-weighted sampling: pdf = cos / pi, so the weight cancels.
        float pdf      = max(cosTheta, 1e-4) / M_PI;
        float saSample = 1.0 / (float(sampleCount) * pdf);
        float mip      = 0.5 * log2(saSample / saTexel);
        color += environment.SampleLevel(L, max(mip, 0.0)).rgb;
    }
    return float4(color / float(sampleCount), 1.0);
}
