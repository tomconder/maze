// skyheight.slang: draws the scene from straight above into an R32F map of the
// highest surface at each point. The probe renders it once at load, and
// skyvis.slang marches rays through it to find how much sky a surface sees.
uniform float4x4 viewProj;
uniform float4x4 model;

// texCoord keeps pbr.slang's attribute location (1) for the mesh VAO.
struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput {
    float4 pos : SV_Position;
    float  height : TEXCOORD0;
};

// clang-format off
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    // clang-format on
    VSOutput output;
    float4   world = mul(model, float4(input.position, 1.0));
    output.pos     = mul(viewProj, world);
    output.height  = world.y;
    return output;
};

// clang-format off
[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    // clang-format on
    return float4(input.height, 0.0, 0.0, 0.0);
};
