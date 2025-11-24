#version 330 core

layout(location = 0) in vec2 position;

uniform mat4 projection;

out VS_OUT {
    vec2 position;
}
vs_out;

void main() {
    gl_Position     = projection * vec4(position, 0.0, 1.0);
    vs_out.position = position;
}
