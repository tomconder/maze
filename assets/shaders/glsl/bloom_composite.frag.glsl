#version 330 core

in  vec2 texCoords;
out vec4 FragColor;

uniform sampler2D scene;
uniform sampler2D bloomTex;
uniform float     intensity;

void main() {
    vec3 color = texture(scene,    texCoords).rgb;
    vec3 bloom = texture(bloomTex, texCoords).rgb;
    FragColor  = vec4(color + bloom * intensity, 1.0);
}
