#version 330 core

// A single-pass wireframe rendering based on a NVIDIA paper published in 2007.
// https://developer.download.nvidia.com/SDK/10/direct3d/Source/SolidWireframe/Doc/SolidWireframe.pdf

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 vPosition[];
in vec2 vTexCoord[];
in vec3 vNormal[];
in vec3 vColor[];

out vec3 gPosition;
out vec2 gTexCoord;
out vec3 gNormal;
out vec3 gColor;
noperspective out vec3 gEdgeDistance;

uniform mat4 viewportMatrix;

void main() {
    vec4 p;

    // transform each vertex into viewport space
    p = gl_in[0].gl_Position;
    vec2 p0 = vec2(viewportMatrix * (p / p.w));

    p = gl_in[1].gl_Position;
    vec2 p1 = vec2(viewportMatrix * (p / p.w));

    p = gl_in[2].gl_Position;
    vec2 p2 = vec2(viewportMatrix * (p / p.w));

    // find the altitudes (ha, hb and hc)
    float a = length(p1 - p2);
    float b = length(p2 - p0);
    float c = length(p1 - p0);

    float alpha = acos((b * b + c * c - a * a) / (2.0 * b * c));
    float beta  = acos((a * a + c * c - b * b) / (2.0 * a * c));

    float ha = abs(c * sin(beta));
    float hb = abs(c * sin(alpha));
    float hc = abs(b * sin(alpha));

    gPosition = vPosition[0];
    gTexCoord = vTexCoord[0];
    gNormal = vNormal[0];
    gColor = vColor[0];
    gEdgeDistance = vec3(ha, 0, 0);
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    gPosition = vPosition[1];
    gTexCoord = vTexCoord[1];
    gNormal = vNormal[1];
    gColor = vColor[1];
    gEdgeDistance = vec3(0, hb, 0);
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    gPosition = vPosition[2];
    gTexCoord = vTexCoord[2];
    gNormal = vNormal[2];
    gColor = vColor[2];
    gEdgeDistance = vec3(0, 0, hc);
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
}
