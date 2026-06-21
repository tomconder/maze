#version 450 core

layout(location = 0, index = 0) out vec4 fragColor;
layout(location = 0, index = 1) out vec4 fragCoverage;

layout(location = 0) in vec2 vTexCoord;

uniform sampler2D text;
uniform vec3      textColor;

void main() {
    vec3 coverage  = texture(text, vTexCoord).rgb;
    fragColor      = vec4(textColor, 1.0);
    fragCoverage   = vec4(coverage, 1.0);
}
