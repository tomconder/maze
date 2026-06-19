#version 330 core

in  vec2 texCoords;
out vec4 FragColor;

uniform sampler2D image;
uniform float     offset;

void main() {
    vec2 halfpixel = 0.5 / vec2(textureSize(image, 0));
    vec2 o         = halfpixel * offset;

    vec3 color = texture(image, texCoords).rgb * 4.0;
    color += texture(image, texCoords + vec2(-o.x, -o.y)).rgb;
    color += texture(image, texCoords + vec2( o.x, -o.y)).rgb;
    color += texture(image, texCoords + vec2(-o.x,  o.y)).rgb;
    color += texture(image, texCoords + vec2( o.x,  o.y)).rgb;

    FragColor = vec4(color / 8.0, 1.0);
}
