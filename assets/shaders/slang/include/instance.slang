// instance.slang: the instance index for GLSL 450 for OpenGL. SV_InstanceID
// becomes gl_InstanceIndex, which only Vulkan GLSL has.
uint instanceIndex() {
    __target_switch {
        case glsl:
            __intrinsic_asm "uint(gl_InstanceID)";
    }
}
