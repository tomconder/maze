#pragma once

struct QuadParams {
    float4x4 projection;  // vertex
    float4   corners;
    float    cornerRadius;
    float4   color;
    float    borderWidth;
    float4   borderColor;
};
[[vk::binding(3)]] ConstantBuffer<QuadParams> quadParams;

struct SpriteParams {
    float4x4 projection;  // vertex
    float    alpha;
    float3   textColor;  // text fragment only
};
[[vk::binding(4)]] ConstantBuffer<SpriteParams> spriteParams;
