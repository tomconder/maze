#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

out VS_OUT {
    vec3 position;
    vec3 normal;
    vec2 texCoords;
    vec4 fragPosLightSpace;
}
vs_out;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main() {
    gl_Position = mvp * vec4(position, 1.0);

    vec4 worldPos    = model * vec4(position, 1.0);
    vs_out.position  = worldPos.xyz;
    vs_out.normal    = transpose(inverse(mat3(model))) * normal;
    vs_out.texCoords = texCoord;

    // transform to light space
    vs_out.fragPosLightSpace = lightSpaceMatrix * worldPos;
}
