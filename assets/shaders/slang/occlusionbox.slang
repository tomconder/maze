// occlusionbox.slang: depth-only AABB proxy for hardware occlusion queries.
// Colour writes are masked off by the caller — the fragment output is never
// read, it only needs to exist for the query's depth test to run.
uniform float4x4 mvp;

struct VSInput {
    float3 position : POSITION;
};
struct VSOutput {
    float4 pos : SV_Position;
};

// clang-format off
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    // clang-format on
    VSOutput o;
    o.pos = mul(mvp, float4(input.position, 1.0));
    return o;
};

// clang-format off
[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    // clang-format on
    return float4(0.0);
};
