#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec4 aInstanceColor;
layout (location = 3) in vec3 aInstanceOffset;
layout (location = 4) in float aInstanceScale;

out vec2 TexCoords;
out vec4 ParticleColor;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aTexCoords;
    ParticleColor = aInstanceColor;
    
    // Billboarding: Extract Right and Up from View Matrix
    vec3 CameraRight_worldspace = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 CameraUp_worldspace = vec3(view[0][1], view[1][1], view[2][1]);
    
    vec3 vertexPosition_worldspace = 
        aInstanceOffset
        + CameraRight_worldspace * aPos.x * aInstanceScale
        + CameraUp_worldspace * aPos.y * aInstanceScale;
        
    gl_Position = projection * view * vec4(vertexPosition_worldspace, 1.0);
}
