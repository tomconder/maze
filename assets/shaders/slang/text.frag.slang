// text.frag.slang — dual-source blending for sub-pixel text rendering
//
// TODO: Slang's GLSL backend emits layout(location=1) for SV_Target1 instead of
// layout(location=0, index=1), which is required for dual-source blending (GL_ARB_blend_func_extended).
// Until Slang supports index-based output layout for GLSL 330, this file is NOT compiled via
// CMakeLists.txt. The hand-written assets/shaders/glsl/text.frag.glsl is used instead.
//
// This file is kept as a reference for the intended Slang translation.
// When Slang gains proper dual-source blending support for GLSL targets, add:
//   compile_slang_shader(text.frag.slang fragMain text.frag.glsl)
// back to CMakeLists.txt and remove assets/shaders/glsl/text.frag.glsl.

Texture2D<float4> text        : register(t0);
SamplerState      textSampler : register(s0);
uniform float3    textColor;

struct FSInput  { float2 texCoord : TEXCOORD0; }

struct FSOutput
{
    float4 fragColor    : SV_Target0;
    float4 fragCoverage : SV_Target1;
}

[shader("fragment")]
FSOutput fragMain(FSInput input)
{
    float3 coverage      = text.Sample(textSampler, input.texCoord).rgb;
    FSOutput output;
    output.fragColor     = float4(textColor, 1.0);
    output.fragCoverage  = float4(coverage,  1.0);
    return output;
}
