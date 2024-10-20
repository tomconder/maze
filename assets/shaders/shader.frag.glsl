#version 330 core

layout (location = 0) out vec4 FragColor;

in vec3 gPosition;
in vec2 gTexCoord;
in vec3 gNormal;
in vec3 gColor;
noperspective in vec3 gEdgeDistance;

uniform sampler2D texture_diffuse1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform bool hasNoTexture;
uniform bool showWireframe;
uniform vec3 lineColor;
uniform float lineWidth;

vec3 blinnPhong(vec3 normal, vec3 fragPos, vec3 lightPos, vec3 color) {
    // diffuse
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;

    // specular
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * color;

    // attenuation
    float distance = clamp(length(lightPos - fragPos), 1.0, 1.5);
    float attenuation = 1.0 / (distance * distance);

    return attenuation * (diffuse + specular);
}

void main() {
    vec3 color;
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    if (hasNoTexture) {
        // color = vec3(1.0, 1.0, 1.0);
        color = gColor;
    } else {
        color = texture(texture_diffuse1, gTexCoord).rgb;
    }

    // ambient
    vec3 ambient = ambientStrength * color;

    vec3 lighting = blinnPhong(normalize(gNormal), gPosition, lightPos, lightColor);
    color *= lighting;

    // gamma
    float gamma = 2.2;
    color = pow(color, vec3(1.0 / gamma));

    float mixVal = 1.0;
    if (showWireframe) {
        float d = min(gEdgeDistance.x, gEdgeDistance.y);
        d = min(d, gEdgeDistance.z);
        mixVal = smoothstep(lineWidth - 1, lineWidth + 1, d);
    }

    FragColor = mix(vec4(lineColor, 1.0), vec4(ambient + color, 1.0), mixVal);
}
