#version 100

#ifdef GL_ES
precision mediump float;
#endif

attribute vec2 position;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(position, 0.0, 1.0);
}
