// skyvis.slang: how much of the sky hemisphere around a surface normal is open,
// from the top-down height map ReflectionProbe renders at load (see
// skyheight.slang). The map holds the highest surface at each point of the
// probe box, so it treats every column as solid below that surface: light that
// enters sideways under a roof is counted as blocked.

static const int   skyDirCount  = 5;
static const int   skyStepCount = 8;
static const float skyFirstStep = 0.3;
static const float skyStepGrow  = 1.8;
// Start off the surface, more than a map texel, so it does not shadow itself.
static const float skyRayOffset = 0.15;

// Cosine-weighted directions in the tangent frame (x, y, normal): the normal
// and four rays 55 degrees off it. The z component is the weight.
static const float3 skyDirs[skyDirCount] = {
    float3(0.0, 0.0, 1.0), float3(0.82, 0.0, 0.57), float3(0.0, 0.82, 0.57),
    float3(-0.82, 0.0, 0.57), float3(0.0, -0.82, 0.57)
};

// 0 is fully covered, 1 sees all the sky above the horizon. Rays that leave the
// map are open. position is world space, normal is the unit surface normal.
float skyVisibility(Sampler2D<float> heightMap, float3 boxMin, float3 boxMax,
                    float3 position, float3 normal) {
    float3 up      = abs(normal.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitan   = cross(normal, tangent);
    float2 size    = boxMax.xz - boxMin.xz;
    float3 origin  = position + normal * skyRayOffset;
    float  weight  = 0.0;
    float  open    = 0.0;

    for (int k = 0; k < skyDirCount; k++) {
        float3 d = tangent * skyDirs[k].x + bitan * skyDirs[k].y + normal * skyDirs[k].z;
        float  w = skyDirs[k].z;
        weight += w;
        // At or below the horizon there is no sky to see.
        if (d.y <= 0.05) {
            continue;
        }
        bool  blocked = false;
        float t       = skyFirstStep;
        for (int s = 0; s < skyStepCount; s++) {
            float3 q = origin + d * t;
            // The map's rows run from boxMax.z (row 0) to boxMin.z.
            float2 uv = float2((q.x - boxMin.x) / size.x, 1.0 - (q.z - boxMin.z) / size.y);
            if (any(uv < 0.0) || any(uv > 1.0)) {
                break;
            }
            if (q.y < heightMap.SampleLevel(uv, 0.0)) {
                blocked = true;
                break;
            }
            t *= skyStepGrow;
        }
        if (!blocked) {
            open += w;
        }
    }
    return open / weight;
}
