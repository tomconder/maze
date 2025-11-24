#version 330 core

in VS_OUT {
    vec2 localPos;
}
fs_in;

out vec4 FragColor;

uniform vec4  color;
uniform float cornerRadius;
uniform vec2  quadSize;

void main() {
    vec2 pos      = fs_in.localPos * quadSize;
    vec2 halfSize = quadSize * 0.5;

    vec2  d    = abs(pos - halfSize) - (halfSize - cornerRadius);
    float dist = length(max(d, 0.0)) - cornerRadius;

    float alpha = smoothstep(1.0, 0.0, dist);

    FragColor = vec4(color.rgb, color.a * alpha);
}
