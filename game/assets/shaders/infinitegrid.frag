#version 330

layout(location = 0) out vec4 outColor;

in vec3 vertexPosition;
in vec3 near;
in vec3 far;
in mat4 viewProj;

vec4 grid(vec3 point, float scale) {
    vec2 coord = point.xz * scale; // use the scale variable to set the distance between the lines
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float min_z = min(derivative.y, 1);
    float min_x = min(derivative.x, 1);

    vec4 color = vec4(0.3);
    color.a = 1.0 - min(line, 1.0);

    if (point.x > -0.1 * min_x && point.x < 0.1 * min_x) {
        // z axis
        color.rgb = vec3(0.427, 0.792, 0.909);
    }

    if (point.z > -0.1 * min_z && point.z < 0.1 * min_z) {
        // x axis
        color.rgb = vec3(0.984, 0.380, 0.490);
    }

    return color;
}

float compute_depth(vec3 point) {
    vec4 clip_space = viewProj * vec4(point, 1.0);
    float clip_space_depth = clip_space.z / clip_space.w;
    float zNear = gl_DepthRange.near;
    float zFar = gl_DepthRange.far;
    return (((zFar - zNear) * clip_space_depth) + zNear + zFar) / 2.0;
}

void main() {
    float t = -near.y / (far.y - near.y);
    vec3 fragPos3D = near + t * (far - near);

    gl_FragDepth = compute_depth(fragPos3D);

    outColor = grid(fragPos3D, 1) * float(t > 0);
}