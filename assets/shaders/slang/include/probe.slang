// probe.slang: helpers shared by the probe convolution shaders.

// Direction through texel (s, t) of a GL cube face. t = 0 is the first row
// of the face, which is the bottom row drawn into the framebuffer.
float3 faceDirection(int f, float2 st) {
    float2 uv = st * 2.0 - 1.0;
    switch (f) {
        case 0:
            return float3(1.0, -uv.y, -uv.x);
        case 1:
            return float3(-1.0, -uv.y, uv.x);
        case 2:
            return float3(uv.x, 1.0, uv.y);
        case 3:
            return float3(uv.x, -1.0, -uv.y);
        case 4:
            return float3(uv.x, -uv.y, 1.0);
        default:
            return float3(-uv.x, -uv.y, -1.0);
    }
}

float2 hammersley(uint i, uint n) {
    return float2(float(i) / float(n), float(reversebits(i)) * 2.3283064365386963e-10);
}
