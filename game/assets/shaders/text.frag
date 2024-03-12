#version 330 core

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D text;
uniform vec3 textColor;
uniform float screenPxRange;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msd = texture2D(text, vTexCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    FragColor = vec4(textColor, 1.0) * vec4(1.0, 1.0, 1.0, opacity);
}
