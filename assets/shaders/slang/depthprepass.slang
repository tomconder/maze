// depthprepass.slang — camera-space depth-only prepass
uniform float4x4 mvp;

struct VSInput  { float3 position : POSITION; }
struct VSOutput { float4 pos : SV_Position; }

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput o;
    o.pos = mul(mvp, float4(input.position, 1.0));
    return o;
}

[shader("fragment")]
void fragMain() {}
