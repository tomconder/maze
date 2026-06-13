#version 330 core

layout(location = 0) out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D image;
uniform bool      horizontal;

const float weight[5] =
    float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 offset = 1.0 / vec2(textureSize(image, 0));
    vec2 result = texture(image, texCoords).rg * weight[0];

    if (horizontal) {
        for (int i = 1; i < 5; ++i) {
            result +=
                texture(image, texCoords + vec2(offset.x * float(i), 0.0)).rg *
                weight[i];
            result +=
                texture(image, texCoords - vec2(offset.x * float(i), 0.0)).rg *
                weight[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            result +=
                texture(image, texCoords + vec2(0.0, offset.y * float(i))).rg *
                weight[i];
            result +=
                texture(image, texCoords - vec2(0.0, offset.y * float(i))).rg *
                weight[i];
        }
    }

    FragColor = vec4(result, 0.0, 1.0);
}
