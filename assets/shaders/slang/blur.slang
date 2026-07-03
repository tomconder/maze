// blur.slang: Dual Kawase blur for EVSM shadow maps (RG channels)
// Sampler2D<float4> is valid for RG32F; hardware pads B=0, A=1
#include "include/kawase.slang"

[[vk::binding(0)]] Sampler2D<float4> image;
uniform float                        offset;

struct FSInput {
    float2 texCoords : TEXCOORD0;
};

// clang-format off
[shader("fragment")]
float4 downFragMain(FSInput input) : SV_Target {
    // clang-format on
    return float4(kawaseDown(image, input.texCoords, offset).rg, 0.0, 1.0);
};

// clang-format off
[shader("fragment")]
float4 upFragMain(FSInput input) : SV_Target {
    // clang-format on
    return float4(kawaseUp(image, input.texCoords, offset).rg, 0.0, 1.0);
};
