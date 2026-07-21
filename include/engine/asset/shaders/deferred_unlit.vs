#version 460 core

layout (location = 0) in vec3 aPos;

layout (location = 1) in vec3 aNormal;

layout (location = 2) in vec2 aTexCoords;

layout (location = 5) in ivec4 aBoneIds; 

layout (location = 6) in vec4 aWeights;



out vec3 FragPos;

out vec3 Normal;

out vec2 TexCoords;



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



layout(location = 10) in mat4 instanceMatrix;
layout(location = 14) in uint instanceEntityID;

uniform bool u_IsInstanced;
uniform uint u_EntityID;

flat out uint EntityID;



const int MAX_BONES = 128;

const int MAX_BONE_INFLUENCE = 4;

uniform mat4 u_FinalBonesMatrices[MAX_BONES];



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

        vec4 localPosition = u_FinalBonesMatrices[aBoneIds[i]] * vec4(aPos,1.0f);

        totalPosition += localPosition * aWeights[i];

        

        vec3 localNormal = mat3(u_FinalBonesMatrices[aBoneIds[i]]) * aNormal;

        totalNormal += localNormal * aWeights[i];

        hasBones = true;

    }

    

    if (!hasBones) {

        totalPosition = vec4(aPos, 1.0f);

        totalNormal = aNormal;

    }



    mat4 modelMatrix = u_IsInstanced ? instanceMatrix : u_Model;
    EntityID = u_IsInstanced ? instanceEntityID : u_EntityID;



    FragPos = vec3(modelMatrix * totalPosition);

    Normal = normalize(mat3(transpose(inverse(modelMatrix))) * totalNormal);  

    TexCoords = aTexCoords;

    

    gl_Position = camera.u_Projection * camera.u_View * vec4(FragPos, 1.0);

}






