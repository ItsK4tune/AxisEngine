#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 5) in ivec4 aBoneIds; 
layout (location = 6) in vec4 aWeights;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout(location = 10) in mat4 instanceMatrix;
uniform bool isInstanced;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

void main()
{
    vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);
    bool hasBones = false;
    for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
    {
        if(aBoneIds[i] == -1) 
            continue;
        if(aBoneIds[i] >= MAX_BONES) 
        {
            totalPosition = vec4(aPos,1.0f);
            totalNormal = aNormal;
            break;
        }
        vec4 localPosition = finalBonesMatrices[aBoneIds[i]] * vec4(aPos,1.0f);
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
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
