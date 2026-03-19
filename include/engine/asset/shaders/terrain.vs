#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;

uniform mat4 model;

// 1. Camera UBO (Standardized Binding 20)
layout(std140, binding = 20) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec3 viewPos;
} camera;

// 2. Heightmap (Standardized Unit 26)
layout (binding = 26) uniform sampler2D heightMap;
uniform float maxHeight;

void main()
{
    TexCoords = aTexCoords;
    
    // Read height from texture
    float height = texture(heightMap, TexCoords).r * maxHeight;
    
    // Displace position
    vec3 displacedPos = aPos;
    displacedPos.y = height;
    
    WorldPos = vec3(model * vec4(displacedPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Use UBO projection/view
    gl_Position = camera.projection * camera.view * vec4(WorldPos, 1.0);
}
