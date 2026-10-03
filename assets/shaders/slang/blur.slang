// blur.slang: Dual Kawase blur for EVSM shadow maps (four moment channels)
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
    return kawaseDown(image, input.texCoords, offset);
};

// clang-format off
[shader("fragment")]
float4 upFragMain(FSInput input) : SV_Target {
    // clang-format on
    return kawaseUp(image, input.texCoords, offset);
};
