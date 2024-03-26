#version 330

layout(location = 0) out vec4 outColor;

in vec3 vertexPosition;
in vec3 near;
in vec3 far;

float checkerboard(vec2 R, float scale) {
    return float((int(floor(R.x / scale)) + int(floor(R.y / scale))) % 2);
}

void main() {
    float t = -near.y / (far.y - near.y);

    vec3 R = near + t * (far - near);

    float c =
      checkerboard(R.xz, 1) * 0.3 +
      checkerboard(R.xz, 10) * 0.2 +
      checkerboard(R.xz, 100) * 0.1 +
      0.1;

    outColor = vec4(vec3(c / 2.0 + 0.3), 1) * float(t > 0);
}
