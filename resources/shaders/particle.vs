#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec4 ParticleColor;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 offset;
uniform vec4 color;
uniform float scale;

void main()
{
    TexCoords = aTexCoords;
    ParticleColor = color;
    
    // Billboarding: Extract Right and Up from View Matrix
    // mat4(view) is [ R U F P ]
    // We want the particle to always face the camera, so we use the inverse of the view rotation
    // OR simper: Just construct the vertex position in View Space.
    
    // Method 1: Spherical Billboarding using view matrix columns
    // vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
    // vec3 cameraUp    = vec3(view[0][1], view[1][1], view[2][1]);
    
    // vec3 vertexPos = offset + cameraRight * aPos.x * scale + cameraUp * aPos.y * scale;
    // gl_Position = projection * view * vec4(vertexPos, 1.0);
    
    // Method 2: Standard MVP but constructing Model matrix manually to face camera
    // This shader receives 'offset' as the center position of particle.
    // 'aPos' are quad coordinates (-0.5 to 0.5).
    
    vec3 CameraRight_worldspace = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 CameraUp_worldspace = vec3(view[0][1], view[1][1], view[2][1]);
    
    vec3 vertexPosition_worldspace = 
        offset
        + CameraRight_worldspace * aPos.x * scale
        + CameraUp_worldspace * aPos.y * scale;
        
    gl_Position = projection * view * vec4(vertexPosition_worldspace, 1.0);
}
