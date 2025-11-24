#version 330 core

in VS_OUT {
    vec2 position;
}
fs_in;

out vec4 FragColor;

uniform vec4  corners;
uniform float cornerRadius;
uniform vec4  color;

void main() {
    // normalize position to [0,1] range within the quad
    vec2 localPos = (fs_in.position - corners.xy) / (corners.zw - corners.xy);

    vec2 quadSize = corners.zw - corners.xy;
    vec2 pos      = localPos * quadSize;
    vec2 halfSize = quadSize * 0.5;

    vec2  d    = abs(pos - halfSize) - (halfSize - cornerRadius);
    float dist = length(max(d, 0.0)) - cornerRadius;

    float alpha = smoothstep(1.0, 0.0, dist);

    FragColor = vec4(color.rgb, color.a * alpha);
}
