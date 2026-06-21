// shadowmap.vert.slang
uniform float4x4 lightSpaceMatrix;
uniform float4x4 model;

struct VSInput
{
    float3 position : POSITION;
}

struct VSOutput
{
    float4 pos : SV_Position;
}

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    output.pos = mul(lightSpaceMatrix, mul(model, float4(input.position, 1.0)));
    return output;
}
