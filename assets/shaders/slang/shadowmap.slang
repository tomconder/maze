// shadowmap.slang — shadow depth vertex + fragment entry points
uniform float4x4 lightSpaceMatrix;
uniform float4x4 model;

struct VSInput
{
    float3 position : POSITION;
}

struct Varyings
{
    float4 pos : SV_Position;
}

[shader("vertex")]
Varyings vertMain(VSInput input)
{
    Varyings output;
    output.pos = mul(lightSpaceMatrix, mul(model, float4(input.position, 1.0)));
    return output;
}

// Plain depth: written implicitly via SV_Position.z — no explicit output needed.
[shader("fragment")]
void fragMain()
{
}

// EVSM: store depth moments (depth, depth²) for variance shadow mapping.
[shader("fragment")]
float4 fragMainEVSM(Varyings input) : SV_Target
{
    float depth = input.pos.z;
    return float4(depth, depth * depth, 0.0, 1.0);
}
