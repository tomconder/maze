// probe_sky.slang: draws the environment image into one face of the probe
// capture, before the scene. The scene then covers it, so the image shows
// only through openings. Drawn with screenquad.vert.
#include "include/probe.slang"

struct FSInput {
    float2 texCoords : TEXCOORD0;
};

// Latitude-longitude (equirectangular) radiance image.
[[vk::binding(0)]] Sampler2D<float4> environment;

uniform int face;

static const float pi = 3.14159265;

// The image holds the sun and bright windows at thousands of times the sky
// radiance. The directional light already lights surfaces, and one such
// texel in a 32 px irradiance face would swamp the rest, so cap it.
static const float maxRadiance = 8.0;

// clang-format off
[shader("fragment")]
float4 fragMain(FSInput input) : SV_Target {
    // clang-format on
    float3 d = normalize(faceDirection(face, input.texCoords));
    // Row 0 of the image is up. SampleLevel, because atan2 jumps at the
    // seam and the hardware's derivatives would pick the smallest mip there.
    float2 uv       = float2(atan2(d.x, -d.z) / (2.0 * pi) + 0.5, acos(d.y) / pi);
    float3 radiance = min(environment.SampleLevel(uv, 0.0).rgb, maxRadiance);
    return float4(radiance, 1.0);
}
