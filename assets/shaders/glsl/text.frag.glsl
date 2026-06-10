#version 330 core

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D text;
uniform vec3      textColor;

void main() {
    float coverage = texture(text, vTexCoord).r;
    FragColor = vec4(textColor, coverage);
}
