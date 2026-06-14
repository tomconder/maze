#version 330 core

layout(location = 0) out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D image;
uniform float     offset;

void main() {
    vec2 halfpixel = 0.5 / vec2(textureSize(image, 0));
    vec2 o         = halfpixel * offset;

    vec2 color = texture(image, texCoords).rg * 4.0;
    color += texture(image, texCoords + vec2(-o.x, -o.y)).rg;
    color += texture(image, texCoords + vec2(o.x, -o.y)).rg;
    color += texture(image, texCoords + vec2(-o.x, o.y)).rg;
    color += texture(image, texCoords + vec2(o.x, o.y)).rg;

    FragColor = vec4(color / 8.0, 0.0, 1.0);
}
