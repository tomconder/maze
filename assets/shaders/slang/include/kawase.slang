#pragma once

float4 kawaseDown(Sampler2D<float4> image, float2 uv, float offset) {
    uint w, h, levels;
    image.GetDimensions(0, w, h, levels);
    float2 o = float2(0.5 / float(w), 0.5 / float(h)) * offset;

    float4 color = image.SampleLevel(uv, 0.0) * 4.0;
    color += image.SampleLevel(uv + float2(-o.x, -o.y), 0.0);
    color += image.SampleLevel(uv + float2(o.x, -o.y), 0.0);
    color += image.SampleLevel(uv + float2(-o.x, o.y), 0.0);
    color += image.SampleLevel(uv + float2(o.x, o.y), 0.0);
    return color / 8.0;
}

float4 kawaseUp(Sampler2D<float4> image, float2 uv, float offset) {
    uint w, h, levels;
    image.GetDimensions(0, w, h, levels);
    float2 o = float2(0.5 / float(w), 0.5 / float(h)) * offset;

    float4 color = float4(0.0);
    color += image.SampleLevel(uv + float2(-o.x * 2.0, 0.0), 0.0);
    color += image.SampleLevel(uv + float2(o.x * 2.0, 0.0), 0.0);
    color += image.SampleLevel(uv + float2(0.0, -o.y * 2.0), 0.0);
    color += image.SampleLevel(uv + float2(0.0, o.y * 2.0), 0.0);
    color += image.SampleLevel(uv + float2(-o.x, o.y), 0.0) * 2.0;
    color += image.SampleLevel(uv + float2(o.x, o.y), 0.0) * 2.0;
    color += image.SampleLevel(uv + float2(-o.x, -o.y), 0.0) * 2.0;
    color += image.SampleLevel(uv + float2(o.x, -o.y), 0.0) * 2.0;
    return color / 12.0;
}
