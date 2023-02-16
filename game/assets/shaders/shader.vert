#version 100

#ifdef GL_ES
precision mediump float;
#endif

attribute vec3 position;
attribute vec3 normal;
attribute vec2 texCoord;

varying vec3 vFragPos;
varying vec3 vNormal;
varying vec2 vTexCoord;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(position, 1.0);

    vNormal = normal;
    vTexCoord = texCoord;
    vFragPos = position;
}
