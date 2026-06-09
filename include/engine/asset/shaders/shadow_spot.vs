#version 460 core

layout (location = 0) in vec3 aPos;

layout (location = 5) in ivec4 aBoneIds;

layout (location = 6) in vec4 aWeights;



uniform mat4 u_LightSpaceMatrix;

uniform mat4 u_Model;



uniform bool u_HasAnimation;

const int MAX_BONES = 200;

uniform mat4 u_FinalBonesMatrices[MAX_BONES];



void main()

{

    vec4 totalPosition = vec4(0.0);

    

    if (u_HasAnimation)

    {

        for(int i = 0; i < 4; i++)

        {

            if(aBoneIds[i] == -1) 

                continue;

            if(aBoneIds[i] >= MAX_BONES) 

            {

                totalPosition = vec4(aPos, 1.0);

                break;

            }

            vec4 localPosition = u_FinalBonesMatrices[aBoneIds[i]] * vec4(aPos, 1.0);

            totalPosition += localPosition * aWeights[i];

        }

        if (totalPosition == vec4(0.0))

            totalPosition = vec4(aPos, 1.0);

    }

    else

    {

        totalPosition = vec4(aPos, 1.0);

    }

    

    gl_Position = u_LightSpaceMatrix * u_Model * totalPosition;

}


