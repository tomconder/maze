// pbr.vert.slang
uniform float4x4 mvp;
uniform float4x4 model;
uniform float4x4 lightSpaceMatrix;

// Slang does not expose inverse() for matrices; compute 3x3 inverse analytically.
float3x3 inverse3x3(float3x3 m)
{
    float3 c0 = float3(m[0][0], m[1][0], m[2][0]);
    float3 c1 = float3(m[0][1], m[1][1], m[2][1]);
    float3 c2 = float3(m[0][2], m[1][2], m[2][2]);
    float3 row0 = cross(c1, c2);
    float3 row1 = cross(c2, c0);
    float3 row2 = cross(c0, c1);
    float det = dot(c0, row0);
    return float3x3(row0 / det, row1 / det, row2 / det);
}

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal   : NORMAL;
}

struct VSOutput
{
    float4 pos              : SV_Position;
    float3 worldPosition    : TEXCOORD0;
    float3 worldNormal      : TEXCOORD1;
    float2 texCoords        : TEXCOORD2;
    float4 fragPosLightSpace: TEXCOORD3;
}

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    float4 worldPos4        = mul(model, float4(input.position, 1.0));
    output.pos              = mul(mvp,   float4(input.position, 1.0));
    output.worldPosition    = worldPos4.xyz;
    // Normal matrix = transpose(inverse(model)) — computed on CPU ideally,
    // but reproduced here to match original shader exactly.
    float3x3 normalMatrix   = transpose(inverse3x3((float3x3)model));
    output.worldNormal      = mul(normalMatrix, input.normal);
    output.texCoords        = input.texCoord;
    output.fragPosLightSpace= mul(lightSpaceMatrix, worldPos4);
    return output;
}
