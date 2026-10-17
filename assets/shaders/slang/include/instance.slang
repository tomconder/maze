#pragma once

// instance.slang: the instance index for GLSL 450 for OpenGL. SV_InstanceID
// becomes gl_InstanceIndex, which only Vulkan GLSL has.
uint instanceIndex() {
    __target_switch {
        case glsl:
            __intrinsic_asm "uint(gl_InstanceID)";
    }
}

uniform float cubeScale;

// Light cubes, one instance each: three float4 per cube (position, previous
// position, colour), each in xyz. Must match Cube::Instance in cube.hpp.
[[vk::binding(8)]] StructuredBuffer<float4> cubeInstances;
