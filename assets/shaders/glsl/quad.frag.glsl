#version 330 core

in VS_OUT {
    vec2 position;
}
fs_in;

out vec4 FragColor;

uniform vec4  corners;
uniform float cornerRadius;
uniform vec4  color;
uniform float borderWidth;
uniform vec4  borderColor;

void main() {
    // normalize position to [0,1] range within the quad
    vec2 localPos = (fs_in.position - corners.xy) / (corners.zw - corners.xy);

    vec2 quadSize = corners.zw - corners.xy;
    vec2 pos      = localPos * quadSize;
    vec2 halfSize = quadSize * 0.5;

    vec2  d    = abs(pos - halfSize) - (halfSize - cornerRadius);
    float dist = length(max(d, 0.0)) - cornerRadius;

    float alpha = smoothstep(1.0, 0.0, dist);

    if (borderWidth > 0.0) {
        // calculate distance from the edge for the inner border
        vec2  innerD    = abs(pos - halfSize) - (halfSize - cornerRadius - borderWidth);
        float innerDist = length(max(innerD, 0.0)) - cornerRadius;

        float borderMask = smoothstep(1.0, 0.0, dist) - smoothstep(1.0, 0.0, innerDist);
        float fillMask   = smoothstep(1.0, 0.0, innerDist);

        vec4 finalColor = borderColor * borderMask + color * fillMask;
        FragColor = vec4(finalColor.rgb, finalColor.a * alpha);
    } else {
        FragColor = vec4(color.rgb, color.a * alpha);
    }
}
