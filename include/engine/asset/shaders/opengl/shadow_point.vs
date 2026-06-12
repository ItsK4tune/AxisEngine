#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 5) in ivec4 aBoneIds;
layout (location = 6) in vec4 aWeights;

uniform mat4 u_Model;
uniform mat4 u_ShadowMatrices[6];

uniform bool u_HasAnimation;
const int MAX_BONES = 200;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 u_FinalBonesMatrices[MAX_BONES];

void main()
{
    vec4 totalPosition = vec4(0.0f);

    if (u_HasAnimation)
    {
        bool hasBones = false;
        for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
        {
            if(aBoneIds[i] == -1)
                continue;
            if(aBoneIds[i] >= MAX_BONES)
            {
                totalPosition = vec4(aPos,1.0f);
                break;
            }
            vec4 localPosition = u_FinalBonesMatrices[aBoneIds[i]] * vec4(aPos,1.0f);
            totalPosition += localPosition * aWeights[i];
            hasBones = true;
        }
        if (!hasBones) {
            totalPosition = vec4(aPos, 1.0f);
        }
    }
    else
    {
        totalPosition = vec4(aPos, 1.0f);
    }

    gl_Position = u_Model * totalPosition;
}
