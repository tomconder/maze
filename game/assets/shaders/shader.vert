#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;

out vec3 vPosition;
out vec2 vTexCoord;
out vec3 vNormal;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(position, 1.0);

    vPosition = position;
    vTexCoord = texCoord;
    vNormal = normal;
}
