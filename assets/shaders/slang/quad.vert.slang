// quad.vert.slang — UI rounded-rect vertex shader
uniform float4x4 projection;

struct VSInput
{
    float2 position : POSITION;
}

struct VSOutput
{
    float4 pos          : SV_Position;
    float2 localPosition: TEXCOORD0;
}

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    output.pos           = mul(projection, float4(input.position, 0.0, 1.0));
    output.localPosition = input.position;
    return output;
}
