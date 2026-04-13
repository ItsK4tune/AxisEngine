#version 430 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

layout(std140, binding = 20) uniform CameraData {
    mat4 u_Projection;
    mat4 u_View;
    vec4 viewPos;
    mat4 u_InvProjection;
    mat4 u_InvView;
    mat4 stableProjection;
    mat4 invStableProjection;
} camera;

void main()
{
    TexCoords = aPos;

    mat4 viewNoTranslation = mat4(mat3(camera.u_View));
    vec4 pos = camera.u_Projection * viewNoTranslation * vec4(aPos, 1.0);
    
    gl_Position = pos.xyww;
}



