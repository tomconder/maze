// cube.slang — debug light-position cube vertex + fragment shader
uniform float4x4 mvp;
uniform float3   lightColor;

struct VSInput  { float3 position : POSITION; }
struct Varyings
{
    float4 pos   : SV_Position;
    float3 color : TEXCOORD0;
}

[shader("vertex")]
Varyings vertMain(VSInput input)
{
    Varyings output;
    output.pos   = mul(mvp, float4(input.position, 1.0));
    output.color = lightColor;
    return output;
}

[shader("fragment")]
float4 fragMain(Varyings input) : SV_Target
{
    return float4(input.color, 1.0);
}
