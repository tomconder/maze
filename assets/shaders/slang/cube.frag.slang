// cube.frag.slang — debug light-position cube fragment shader
struct FSInput  { float3 color : TEXCOORD0; }

[shader("fragment")]
float4 fragMain(FSInput input) : SV_Target
{
    return float4(input.color, 1.0);
}
