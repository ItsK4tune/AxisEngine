#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 LineColor;

layout(std140, binding = 20) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec4 viewPos;
    mat4 invProjection;
    mat4 invView;
    mat4 stableProjection;
    mat4 invStableProjection;
} camera;

void main()
{
    LineColor = aColor;
    gl_Position = camera.projection * camera.view * vec4(aPos, 1.0);
}



