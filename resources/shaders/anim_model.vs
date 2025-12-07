#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tex;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;
layout(location = 5) in ivec4 boneIds; 
layout(location = 6) in vec4 weights;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

const int MAX_BONES = 100;
uniform mat4 finalBonesMatrices[MAX_BONES];

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

void main()
{
    vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f); // Tích lũy Normal

    // Kiểm tra xem vertex có trọng số xương không (nếu tổng weight ~ 0 thì không animation)
    float weightSum = weights.x + weights.y + weights.z + weights.w;
    
    if (weightSum > 0.0) // Có animation
    {
        for(int i = 0 ; i < 4 ; i++)
        {
            if(boneIds[i] == -1) 
                continue;
            
            if(boneIds[i] >= MAX_BONES) 
            {
                // Xử lý lỗi an toàn
                totalPosition = vec4(pos, 1.0f);
                totalNormal = norm;
                break;
            }

            // Vị trí
            vec4 localPosition = finalBonesMatrices[boneIds[i]] * vec4(pos, 1.0f);
            totalPosition += localPosition * weights[i];

            // Pháp tuyến (Normal) - Quan trọng cho ánh sáng
            // Lưu ý: Chỉ dùng mat3 để xoay normal, không dịch chuyển
            vec3 localNormal = mat3(finalBonesMatrices[boneIds[i]]) * norm;
            totalNormal += localNormal * weights[i];
        }
    }
    else // Không có animation (Static mesh)
    {
        totalPosition = vec4(pos, 1.0f);
        totalNormal = norm;
    }
	
    mat4 viewModel = view * model;
    gl_Position =  projection * viewModel * totalPosition;
	
    TexCoords = tex;
    // Chuyển Normal sang World Space
    Normal = normalize(mat3(transpose(inverse(model))) * totalNormal);
    FragPos = vec3(model * totalPosition);
}
