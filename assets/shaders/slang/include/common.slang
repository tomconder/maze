#pragma once

static const float M_PI = 3.14159265359;

// Attenuation coefficients [constant, linear, quadratic] per distance range.
// See https://learnopengl.com/Lighting/Light-casters
static const float3 kLightAttenuation[11] = {
    float3(1.0, 0.70,  1.800), float3(1.0, 0.35,  0.440),
    float3(1.0, 0.22,  0.200), float3(1.0, 0.14,  0.070),
    float3(1.0, 0.09,  0.032), float3(1.0, 0.07,  0.017),
    float3(1.0, 0.045, 0.0075),float3(1.0, 0.027, 0.0028),
    float3(1.0, 0.022, 0.0019),float3(1.0, 0.014, 0.0007),
    float3(1.0, 0.007, 0.0002)
};
