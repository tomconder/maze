#version 330 core

layout(location = 0) in vec2 position;

uniform mat4 projection;
uniform vec2 quadMin;
uniform vec2 quadMax;

out VS_OUT {
    vec2 localPos;
}
vs_out;

void main() {
    gl_Position = projection * vec4(position, 0.0, 1.0);

    // normalize position to [0,1] range within the quad
    vs_out.localPos = (position - quadMin) / (quadMax - quadMin);
}
