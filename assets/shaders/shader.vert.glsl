#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 color;

out vec3 vPosition;
out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vColor;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(position, 1.0);

    vPosition = position;
    vTexCoord = texCoord;
    vNormal = normal;
    vColor = color;
}
