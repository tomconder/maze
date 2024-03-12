#version 330 core

layout (location = 0) in vec4 vertex;

out vec2 vTexCoord;

uniform mat4 projection;

void main() {
    vTexCoord = vertex.zw;
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
}
