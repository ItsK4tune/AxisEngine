#version 430 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

layout(std140, binding = 20) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec3 viewPos;
} camera;

void main()
{
    TexCoords = aPos;

    mat4 viewNoTranslation = mat4(mat3(camera.view));
    vec4 pos = camera.projection * viewNoTranslation * vec4(aPos, 1.0);
    
    gl_Position = pos.xyww;
}
