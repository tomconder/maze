// ssr.slang: screen-space reflection. The prepass normal alpha holds each
// pixel's reflection strength. For a pixel above 0, march the reflected view
// ray across the screen in fixed pixel strides (McGuire and Mara's DDA march)
// against the prepass depth, refine the hit with a binary search, and return
// the scene color there with the weight in alpha. The caller alpha-blends
// the result over the lit surface. A ray that only passed behind objects
// shows the first one it passed; a ray that found nothing leaves the surface
// as it is.
struct FSInput {
    float2 texCoords : TEXCOORD0;
};

[[vk::binding(0)]] Sampler2D<float4> sceneColor;  // copy of the HDR scene color
[[vk::binding(1)]] Sampler2D<float>  depthTex;
[[vk::binding(2)]] Sampler2D<float4> normalTex;  // xyz view normal, w strength

uniform float4x4 projection;
uniform float4x4 invProjection;

// Longest march before giving up, in screen-space strides.
static const int maxSteps = 256;
// Screen-space step size, in pixels. Stepping in pixels spreads samples
// evenly across the screen, unlike a fixed view-space step.
static const float stride = 4.0;
// Binary-search steps used to refine a hit inside one stride.
static const int refineSteps = 5;
// Ray length clip and fade-out distance, view-space units.
static const float maxDistance = 100.0;
// A stride hits when its far end has passed behind the visible surface and
// its near end is at most this many view-space units behind it. Past that,
// the ray has gone behind an object rather than landing on it, so the march
// goes on.
static const float thickness = 0.3;

float3 viewPositionFromDepth(float2 uv, float depth) {
    float4 clip = float4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    float4 view = mul(invProjection, clip);
    return view.xyz / view.w;
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
    float4 normalStrength = normalTex.SampleLevel(input.texCoords, 0.0);
    float  strength       = normalStrength.w;
    if (strength <= 0.0) {
        discard;
    }

    float3 N = normalize(normalStrength.xyz);
    float3 P = viewPositionFromDepth(input.texCoords,
                                     depthTex.SampleLevel(input.texCoords, 0.0));
    float3 R = reflect(normalize(P), N);

    // The camera looks down -z. A ray toward the camera reaches surfaces the
    // camera cannot see, so it fades out.
    float towardCamera = 1.0 - smoothstep(0.0, 0.4, R.z);
    if (towardCamera <= 0.0) {
        discard;
    }

    float2 size;
    depthTex.GetDimensions(size.x, size.y);

    // Clip the ray just in front of the near plane, so its end projects in
    // front of the camera. View z is negative in front of the camera.
    float nearZ     = viewPositionFromDepth(float2(0.5), 0.0).z;
    float rayLength = maxDistance;
    if (R.z > 0.0 && P.z + R.z * maxDistance > nearZ - 1e-3) {
        rayLength = (nearZ - 1e-3 - P.z) / R.z;
    }

    // Project both ends. Q / k and k = 1/w interpolate linearly in screen
    // space, so the view position is correct at every pixel on the way.
    float4 H0 = mul(projection, float4(P, 1.0));
    float4 H1 = mul(projection, float4(P + R * rayLength, 1.0));
    float  k0 = 1.0 / H0.w;
    float  k1 = 1.0 / H1.w;
    float3 Q0 = P * k0;
    float3 Q1 = (P + R * rayLength) * k1;
    float2 P0 = (H0.xy * k0 * 0.5 + 0.5) * size;
    float2 P1 = (H1.xy * k1 * 0.5 + 0.5) * size;
    // A ray that stays on one pixel still needs a direction.
    if (dot(P1 - P0, P1 - P0) < 1e-4) {
        P1 += 0.01;
    }

    // Step along the longer screen axis, one stride at a time.
    float2 delta   = P1 - P0;
    bool   permute = abs(delta.x) < abs(delta.y);
    if (permute) {
        delta = delta.yx;
        P0    = P0.yx;
        P1    = P1.yx;
    }
    float  stepDir = sign(delta.x);
    float  invdx   = stepDir / delta.x;
    float2 dP      = float2(stepDir, delta.y * invdx) * stride;
    float3 dQ      = (Q1 - Q0) * invdx * stride;
    float  dk      = (k1 - k0) * invdx * stride;
    float  endX    = P1.x * stepDir;

    float2 Pc     = P0;
    float3 Qc     = Q0;
    float  kc     = k0;
    float  prevZ  = P.z;
    bool   hit    = false;
    float3 hitPos = float3(0.0);
    float2 hitUV  = float2(0.0);
    // The first point where the ray passed behind an object by more than
    // thickness. If the march finds no real hit, the reflection shows that
    // object instead of a hole.
    bool   behindFound = false;
    float3 behindPos   = float3(0.0);
    float2 behindUV    = float2(0.0);

    for (int i = 0; i < maxSteps; i++) {
        float2 Pnext = Pc + dP;
        float3 Qn    = Qc + dQ;
        float  kn    = kc + dk;
        if (Pnext.x * stepDir > endX) {
            break;
        }
        float2 uv = (permute ? Pnext.yx : Pnext) / size;
        if (!onScreen(uv)) {
            break;
        }
        // It hits when the step's far end has passed behind the surface and
        // its near end, max(prevZ, rayZ), is at most thickness behind it.
        float rayZ   = Qn.z / kn;
        float sceneZ = sceneViewZ(uv);
        if (min(prevZ, rayZ) <= sceneZ && max(prevZ, rayZ) >= sceneZ - thickness) {
            // Binary search inside this step for the first point behind the
            // surface.
            float2 Pa = Pc;
            float3 Qa = Qc;
            float  ka = kc;
            float2 Pb = Pnext;
            float3 Qb = Qn;
            float  kb = kn;
            for (int j = 0; j < refineSteps; j++) {
                float2 Pm = (Pa + Pb) * 0.5;
                float3 Qm = (Qa + Qb) * 0.5;
                float  km = (ka + kb) * 0.5;
                float2 m  = (permute ? Pm.yx : Pm) / size;
                if (sceneViewZ(m) > Qm.z / km) {
                    Pb = Pm;
                    Qb = Qm;
                    kb = km;
                } else {
                    Pa = Pm;
                    Qa = Qm;
                    ka = km;
                }
            }
            hitUV  = (permute ? Pb.yx : Pb) / size;
            hitPos = Qb / kb;
            hit    = true;
            break;
        }
        if (!behindFound && rayZ < sceneZ) {
            behindFound = true;
            behindPos   = Qn / kn;
            behindUV    = uv;
        }
        prevZ = rayZ;
        Pc    = Pnext;
        Qc    = Qn;
        kc    = kn;
    }

    if (!hit) {
        if (!behindFound) {
            discard;
        }
        hitPos = behindPos;
        hitUV  = behindUV;
    }

    float2 edge         = smoothstep(float2(0.0), float2(0.05), hitUV) *
                          smoothstep(float2(0.0), float2(0.05), 1.0 - hitUV);
    float  distanceFade = 1.0 - smoothstep(0.75 * maxDistance, maxDistance, length(hitPos - P));
    float  weight       = strength * edge.x * edge.y * distanceFade * towardCamera;

    return float4(sceneColor.SampleLevel(hitUV, 0.0).rgb, weight);
};
