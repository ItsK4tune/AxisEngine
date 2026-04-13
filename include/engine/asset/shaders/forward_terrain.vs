#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

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


layout (binding = 26) uniform sampler2D heightMap;
uniform float maxHeight;

void main()
{
    TexCoords = aTexCoords;
    

    float height = texture(heightMap, TexCoords).r * maxHeight;
    

    vec3 displacedPos = aPos;
    displacedPos.y = height;
    
    WorldPos = vec3(u_Model * vec4(displacedPos, 1.0));
    Normal = mat3(transpose(inverse(u_Model))) * aNormal;
    

    gl_Position = camera.u_Projection * camera.u_View * vec4(WorldPos, 1.0);
}



