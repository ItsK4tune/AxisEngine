#version 430 core

// --- AxisEngine Standard Vertex Shader Template ---

// 1. Vertex Attributes (Standard Locations)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIds; 
layout (location = 6) in vec4 aWeights;

// Instancing support
layout (location = 10) in mat4 instanceMatrix; 

// 2. Outputs to Next Stage
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 Tangent;
out vec3 Bitangent;
out vec4 FragPosLightSpace[2];
out vec4 FragPosLightSpaceSpot[2];

// 3. UBO Bindings (Standardized Range 20-22)
layout(std140, binding = 20) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec3 viewPos;
} camera;

layout(std140, binding = 21) uniform LightData {
    mat4 lightSpaceMatricesDir[2];
    mat4 lightSpaceMatricesSpot[2];
    int numDirLights;
    int nrPointLights;
    int nrSpotLights;
    int u_ReceiveShadow;
    float farPlanePoint;
    float farPlaneSpot;
} light;

layout(std140, binding = 22) uniform GlobalData {
    float u_Time;
    float u_DeltaTime;
    vec2  u_Resolution;
} globalData;

// 4. Uniforms
uniform mat4 model;
uniform bool isInstanced;
uniform vec2 u_UVScale = vec2(1.0);
uniform vec2 u_UVOffset = vec2(0.0);

const int MAX_BONES = 200;
uniform mat4 finalBonesMatrices[MAX_BONES];

void main()
{
    mat4 modelMatrix = isInstanced ? instanceMatrix : model;
    
    // Default position calculation (Add animation logic if needed)
    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(modelMatrix))) * aNormal;
    TexCoords = aTexCoords * u_UVScale + u_UVOffset;
    
    // Shadow space calculations
    for(int i = 0; i < 2; i++)
        FragPosLightSpace[i] = light.lightSpaceMatricesDir[i] * vec4(FragPos, 1.0);
        
    gl_Position = camera.projection * camera.view * vec4(FragPos, 1.0);
}
