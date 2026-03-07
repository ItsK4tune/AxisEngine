#version 430 core
layout (location = 0) in vec3 aPos;

layout(std140, binding = 0) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec3 viewPos;
} camera;

uniform mat4 model;

void main()
{
    gl_Position = camera.projection * camera.view * model * vec4(aPos, 1.0);
}
