#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec4 aInstanceColor;
layout (location = 3) in vec3 aInstanceOffset;
layout (location = 4) in float aInstanceScale;

out vec2 TexCoords;
out vec4 ParticleColor;

layout(std140, binding = 0) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec3 viewPos;
} camera;

void main()
{
    TexCoords = aTexCoords;
    ParticleColor = aInstanceColor;
    
    // Billboarding: Extract Right and Up from View Matrix
    vec3 CameraRight_worldspace = vec3(camera.view[0][0], camera.view[1][0], camera.view[2][0]);
    vec3 CameraUp_worldspace = vec3(camera.view[0][1], camera.view[1][1], camera.view[2][1]);
    
    vec3 vertexPosition_worldspace = 
        aInstanceOffset
        + CameraRight_worldspace * aPos.x * aInstanceScale
        + CameraUp_worldspace * aPos.y * aInstanceScale;
        
    gl_Position = camera.projection * camera.view * vec4(vertexPosition_worldspace, 1.0);
}
