// sprite.vert.slang — sprite and text vertex shader
uniform float4x4 projection;

struct VSInput
{
    float4 vertex : POSITION; // xy=pos, zw=texcoord
}

struct VSOutput
{
    float4 pos      : SV_Position;
    float2 texCoord : TEXCOORD0;
}

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    output.texCoord = input.vertex.zw;
    output.pos      = mul(projection, float4(input.vertex.xy, 0.0, 1.0));
    return output;
}
