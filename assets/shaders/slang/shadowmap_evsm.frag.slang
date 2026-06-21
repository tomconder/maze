// shadowmap_evsm.frag.slang
struct FSInput
{
    float4 fragPos : SV_Position;
}

struct FSOutput
{
    float4 color : SV_Target;
}

[shader("fragment")]
FSOutput fragMain(FSInput input)
{
    float depth  = input.fragPos.z;
    FSOutput output;
    output.color = float4(depth, depth * depth, 0.0, 1.0);
    return output;
}
