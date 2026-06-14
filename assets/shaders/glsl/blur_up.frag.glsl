#version 330 core

layout(location = 0) out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D image;
uniform float     offset;

void main() {
    vec2 halfpixel = 0.5 / vec2(textureSize(image, 0));
    vec2 o         = halfpixel * offset;

    vec2 color = vec2(0.0);
    color += texture(image, texCoords + vec2(-o.x * 2.0, 0.0)).rg;
    color += texture(image, texCoords + vec2(o.x * 2.0, 0.0)).rg;
    color += texture(image, texCoords + vec2(0.0, -o.y * 2.0)).rg;
    color += texture(image, texCoords + vec2(0.0, o.y * 2.0)).rg;
    color += texture(image, texCoords + vec2(-o.x, o.y)).rg * 2.0;
    color += texture(image, texCoords + vec2(o.x, o.y)).rg * 2.0;
    color += texture(image, texCoords + vec2(-o.x, -o.y)).rg * 2.0;
    color += texture(image, texCoords + vec2(o.x, -o.y)).rg * 2.0;

    FragColor = vec4(color / 12.0, 0.0, 1.0);
}
