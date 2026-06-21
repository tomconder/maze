// cube.vert.slang — debug light-position cube vertex shader
uniform float4x4 mvp;
uniform float3   lightColor;

struct VSInput  { float3 position : POSITION; }
struct VSOutput
{
    float4 pos   : SV_Position;
    float3 color : TEXCOORD0;
}

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    output.pos   = mul(mvp, float4(input.position, 1.0));
    output.color = lightColor;
    return output;
}
