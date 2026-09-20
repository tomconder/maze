// ssao.slang: screen-space ambient occlusion, hemisphere-kernel sampling
// against the depth prepass's depth + view-space normal targets. Technique
// follows the classic Crytek/Rendu approach (github.com/kosua20/Rendu,
// resources/common/shaders/screens/ssao.frag): reconstruct view-space
// position from depth, orient a hemisphere kernel with a random per-pixel
// rotation, and count how many kernel samples land behind real geometry.
struct FSInput {
    float2 texCoords : TEXCOORD0;
};

[[vk::binding(0)]] Sampler2D<float>  depthTex;
[[vk::binding(1)]] Sampler2D<float4> normalTex;  // view-space normal; (0,0,0) = no surface (background/light cubes)
[[vk::binding(2)]] Sampler2D<float4> noiseTex;   // 4x4 tiling rotation vectors

uniform float4x4 projection;
uniform float4x4 invProjection;
uniform float2   noiseScale;  // screenSize / 4, tiles noiseTex across the screen
uniform float    radius;

// Without this, a flat unoccluded surface still darkens by roughly half:
// depth-buffer/normal quantization noise makes about half the kernel samples
// read back as "behind" the very surface they were cast from. The bias
// demands the occluder sit meaningfully closer than the sample point before
// it counts.
static const float bias = 0.025;

// 16 hemisphere sample offsets, cosine-weighted and biased toward the
// origin (more samples close to the surface, where occlusion detail is),
// precomputed offline with a fixed seed. Scaled by `radius` below.
static const float3 kernel[16] = {
    float3(0.031343, 0.008854, 0.048690),
    float3(-0.054242, 0.052629, 0.031128),
    float3(0.014520, -0.006623, 0.017081),
    float3(0.037989, 0.100225, 0.013283),
    float3(-0.009715, -0.041701, 0.033541),
    float3(-0.023567, 0.073727, 0.097785),
    float3(0.173635, 0.049325, 0.120350),
    float3(0.149972, -0.094466, 0.175101),
    float3(-0.003614, -0.019836, 0.008477),
    float3(0.001372, 0.006399, 0.015564),
    float3(-0.078911, 0.130415, 0.264265),
    float3(0.088872, -0.199457, 0.093786),
    float3(0.374838, 0.268386, 0.141412),
    float3(0.101602, -0.062204, 0.362429),
    float3(0.431340, 0.086597, 0.504608),
    float3(-0.219564, 0.363803, 0.202840),
};

float3 viewPositionFromDepth(float2 uv, float depth) {
    float4 clip = float4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    float4 view = mul(invProjection, clip);
    return view.xyz / view.w;
}

// clang-format off
[shader("fragment")]
float fragMain(FSInput input) : SV_Target {
    // clang-format on
    float3 n = normalTex.Sample(input.texCoords).xyz;
    if (length(n) < 0.1) {
        return 1.0;
    }

    float3 randomVec = normalize(float3(noiseTex.Sample(input.texCoords * noiseScale).xy, 0.0));
    float3 t         = normalize(randomVec - n * dot(randomVec, n));
    float3 b         = cross(n, t);

    float  depth    = depthTex.Sample(input.texCoords).r;
    float3 position = viewPositionFromDepth(input.texCoords, depth);

    float occlusion = 0.0;
    for (int i = 0; i < 16; ++i) {
        float3 offset    = t * kernel[i].x + b * kernel[i].y + n * kernel[i].z;
        float3 samplePos = position + radius * offset;

        float4 sampleClip = mul(projection, float4(samplePos, 1.0));
        float2 sampleUV   = (sampleClip.xy / sampleClip.w) * 0.5 + 0.5;

        float  sampleDepth = depthTex.Sample(sampleUV).r;
        float3 sampledPos  = viewPositionFromDepth(sampleUV, sampleDepth);

        float isValid = abs(position.z - sampledPos.z) < radius ? 1.0 : 0.0;
        occlusion += (sampledPos.z >= samplePos.z + bias) ? isValid : 0.0;
    }

    return 1.0 - occlusion / 16.0;
};
