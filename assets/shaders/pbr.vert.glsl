#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;

out vec3 vPosition;
out vec2 vTexCoord;
out vec3 vNormal;
out vec4 vFragPosLightSpace;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main() {
    vec4 worldPos = model * vec4(position, 1.0);
    gl_Position = mvp * vec4(position, 1.0);

    vPosition = worldPos.xyz;
    vTexCoord = texCoord;
    vNormal = mat3(transpose(inverse(model))) * normal;

    // transform to light space
    vFragPosLightSpace = lightSpaceMatrix * worldPos;
}
