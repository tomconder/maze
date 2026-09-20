// ssaoblur.slang: depth-aware box blur over the raw SSAO pass. Plain box
// blur would smear occlusion across depth discontinuities (an object's edge
// bleeding onto the background behind it); weighting each tap by how close
// its depth is to the center pixel's keeps edges sharp.
struct FSInput {
    float2 texCoords : TEXCOORD0;
};

[[vk::binding(0)]] Sampler2D<float> aoTex;
[[vk::binding(1)]] Sampler2D<float> depthTex;

uniform float2 texelSize;  // 1 / ssao target resolution

// clang-format off
[shader("fragment")]
float fragMain(FSInput input) : SV_Target {
    // clang-format on
    float centerDepth = depthTex.Sample(input.texCoords).r;

    float total  = 0.0;
    float weight = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float2 uv    = input.texCoords + float2(x, y) * texelSize;
            float  depth = depthTex.Sample(uv).r;
            // Depth is nonlinear (raw buffer values), but the tap footprint
            // is only 3x3 texels, so the resulting depth-difference bias
            // stays negligible without linearizing.
            float w = 1.0 / (1e-5 + abs(depth - centerDepth));
            total += aoTex.Sample(uv).r * w;
            weight += w;
        }
    }

    return total / weight;
};
