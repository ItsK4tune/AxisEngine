#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

layout (location = 5) in ivec4 aBoneIds; 
layout (location = 6) in vec4 aWeights;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

layout(std140, binding = 20) uniform CameraData {
    mat4 projection;
    mat4 view;   
    vec3 viewPos;
} camera;

uniform mat4 model;
uniform mat4 finalBonesMatrices[200];
uniform bool isInstanced;
layout(location = 10) in mat4 instanceMatrix;

void main()
{
    vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);
    bool hasBones = false;
    
    for(int i = 0 ; i < 4 ; i++)
    {
        if(aBoneIds[i] == -1) continue;
        if(aBoneIds[i] >= 200) {
            totalPosition = vec4(aPos, 1.0f);
            totalNormal = aNormal;
            break;
        }
        vec4 localPosition = finalBonesMatrices[aBoneIds[i]] * vec4(aPos, 1.0f);
        totalPosition += localPosition * aWeights[i];
        vec3 localNormal = mat3(finalBonesMatrices[aBoneIds[i]]) * aNormal;
        totalNormal += localNormal * aWeights[i];
        hasBones = true;
    }
    
    if (!hasBones) {
        totalPosition = vec4(aPos, 1.0f);
        totalNormal = aNormal;
    }

    mat4 modelMatrix = isInstanced ? instanceMatrix : model;

    FragPos = vec3(modelMatrix * totalPosition);
    Normal = mat3(transpose(inverse(modelMatrix))) * totalNormal;  
    TexCoords = aTexCoords;
    
    gl_Position = camera.projection * camera.view * vec4(FragPos, 1.0);
}
