#version 460 core

layout (location = 0) in vec3 aPos;

layout (location = 2) in vec2 aTexCoords;

uniform mat4 u_Model;

layout(std140, binding = 20) uniform CameraData {
    mat4 u_Projection;
    mat4 u_View;
    vec4 viewPos;
    mat4 u_InvProjection;
    mat4 u_InvView;
    mat4 stableProjection;
    mat4 invStableProjection;
} camera;

out vec2 TexCoords;

void main() {

    TexCoords = aTexCoords;

    gl_Position = camera.u_Projection * camera.u_View * u_Model * vec4(aPos, 1.0);

}






