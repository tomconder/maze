#version 330 core

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D image;

void main() {
    FragColor = texture(image, vTexCoord);
    if (FragColor == vec4(0.0)) {
        discard;
    }
}
