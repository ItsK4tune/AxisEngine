#version 430 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

// 1. UBO Bindings (Range 20-22)
layout(std140, binding = 20) uniform CameraData {
    mat4 u_Projection;
    mat4 u_View;
    vec3 viewPos;
} camera;

void main() {
    for(int i = 0; i < 3; i++) {
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}
