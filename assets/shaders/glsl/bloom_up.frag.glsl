#version 330 core

in  vec2 texCoords;
out vec4 FragColor;

uniform sampler2D image;
uniform float     offset;

void main() {
    vec2 halfpixel = 0.5 / vec2(textureSize(image, 0));
    vec2 o         = halfpixel * offset;

    vec3 color = vec3(0.0);
    color += texture(image, texCoords + vec2(-o.x * 2.0, 0.0)).rgb;
    color += texture(image, texCoords + vec2( o.x * 2.0, 0.0)).rgb;
    color += texture(image, texCoords + vec2(0.0, -o.y * 2.0)).rgb;
    color += texture(image, texCoords + vec2(0.0,  o.y * 2.0)).rgb;
    color += texture(image, texCoords + vec2(-o.x,  o.y)).rgb * 2.0;
    color += texture(image, texCoords + vec2( o.x,  o.y)).rgb * 2.0;
    color += texture(image, texCoords + vec2(-o.x, -o.y)).rgb * 2.0;
    color += texture(image, texCoords + vec2( o.x, -o.y)).rgb * 2.0;

    FragColor = vec4(color / 12.0, 1.0);
}
