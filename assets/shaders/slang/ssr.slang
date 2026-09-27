// ssr.slang: screen-space reflection. The prepass normal alpha holds each
// pixel's reflection strength. For a pixel above 0, march the reflected view
// ray in view space against the prepass depth, refine the hit with a binary
// search, and return the scene color there with the weight in alpha. The
// caller alpha-blends the result over the lit surface, so a miss leaves the
// surface as it is.
struct FSInput {
    float2 texCoords : TEXCOORD0;
};

[[vk::binding(0)]] Sampler2D<float4> sceneColor;  // copy of the HDR scene color
[[vk::binding(1)]] Sampler2D<float>  depthTex;
[[vk::binding(2)]] Sampler2D<float4> normalTex;  // xyz view normal, w strength

uniform float4x4 projection;
uniform float4x4 invProjection;

static const int maxSteps    = 64;
static const int refineSteps = 5;
// View-space units. The step length is maxDistance / maxSteps, about 1.56.
static const float maxDistance = 100.0;
// After the binary search, a hit this far or more behind the depth buffer is
// behind an object, not on it, so the march goes on. The search runs on the
// first step that lands behind the depth buffer, whatever the gap: one step
// (maxDistance / maxSteps, about 1.56) can be longer than this, and testing
// the raw gap would let the ray pass through a surface.
static const float thickness = 0.3;

float3 viewPositionFromDepth(float2 uv, float depth) {
    float4 clip = float4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    float4 view = mul(invProjection, clip);
    return view.xyz / view.w;
}

float2 projectToUV(float3 viewPos) {
    float4 clip = mul(projection, float4(viewPos, 1.0));
    return clip.xy / clip.w * 0.5 + 0.5;
}

// Level 0 explicitly: the march loop has no usable derivatives.
float sceneViewZ(float2 uv) {
    return viewPositionFromDepth(uv, depthTex.SampleLevel(uv, 0.0)).z;
}

bool onScreen(float2 uv) {
    return all(uv >= 0.0) && all(uv <= 1.0);
}

// clang-format off
[shader("fragment")]
float4 fragMain(FSInput input) : SV_Target {
    // clang-format on
    float4 normalStrength = normalTex.Sample(input.texCoords);
    float  strength       = normalStrength.w;
    if (strength <= 0.0) {
        discard;
    }

    float3 N = normalize(normalStrength.xyz);
    float3 P = viewPositionFromDepth(input.texCoords, depthTex.Sample(input.texCoords));
    float3 R = reflect(normalize(P), N);

    // The camera looks down -z. A ray toward the camera reaches surfaces the
    // camera cannot see, so it fades out.
    float towardCamera = 1.0 - smoothstep(0.0, 0.4, R.z);
    if (towardCamera <= 0.0) {
        discard;
    }

    float  stepLength = maxDistance / float(maxSteps);
    float3 prev       = P;
    float3 ray        = P + R * stepLength;
    bool   hit        = false;
    float3 hitPos     = float3(0.0);

    for (int i = 0; i < maxSteps; i++) {
        if (ray.z >= 0.0) {
            break;
        }
        float2 uv = projectToUV(ray);
        if (!onScreen(uv)) {
            break;
        }
        // Above 0 when the ray is behind the visible surface.
        if (sceneViewZ(uv) - ray.z > 0.0) {
            float3 a = prev;
            float3 b = ray;
            for (int j = 0; j < refineSteps; j++) {
                float3 mid = (a + b) * 0.5;
                if (sceneViewZ(projectToUV(mid)) - mid.z > 0.0) {
                    b = mid;
                } else {
                    a = mid;
                }
            }
            if (sceneViewZ(projectToUV(b)) - b.z < thickness) {
                hitPos = b;
                hit    = true;
                break;
            }
        }
        prev = ray;
        ray += R * stepLength;
    }

    if (!hit) {
        discard;
    }

    float2 hitUV        = projectToUV(hitPos);
    float2 edge         = smoothstep(float2(0.0), float2(0.05), hitUV) *
                          smoothstep(float2(0.0), float2(0.05), 1.0 - hitUV);
    float  distanceFade = 1.0 - smoothstep(0.75 * maxDistance, maxDistance, length(hitPos - P));
    float  weight       = strength * edge.x * edge.y * distanceFade * towardCamera;

    return float4(sceneColor.SampleLevel(hitUV, 0.0).rgb, weight);
};
