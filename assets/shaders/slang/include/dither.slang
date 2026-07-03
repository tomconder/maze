#pragma once

// Interleaved gradient noise (Jimenez 2014), ±0.5 LSB at 8-bit — breaks up
// banding in dim gradients. Apply at every pass that writes an 8-bit target.
float3 dither8(float3 color, float2 fragCoord)
{
    float ign = frac(52.9829189 *
                     frac(dot(fragCoord, float2(0.06711056, 0.00583715))));
    return color + (ign - 0.5) / 255.0;
}
