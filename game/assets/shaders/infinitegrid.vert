#version 330

layout (location = 0) in vec3 position;

out vec3 vertexPosition;
out vec3 near;
out vec3 far;

uniform mat4 mvp;

vec3 unproject_point(float x, float y, float z) {
    mat4 inv = inverse(mvp);
    vec4 unproj_point = inv * vec4(x, y, z, 1.f);
    return unproj_point.xyz / unproj_point.w;
}

void main() {
    gl_Position = vec4(position, 1.0);

    vertexPosition = position;

    vec2 p = position.xy;
    near = unproject_point(p.x, p.y, -1.f);
    far  = unproject_point(p.x, p.y,  1.f);
}
