// screenquad.slang — fullscreen quad passthrough

struct VSInput
{
    float2 position  : POSITION;
    float2 texCoord  : TEXCOORD0;
};

struct VSOutput
{
    float4 pos       : SV_Position;
    float2 texCoords : TEXCOORD0;
};

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    output.pos       = float4(input.position, 0.0, 1.0);
    output.texCoords = input.texCoord;
    return output;
}
