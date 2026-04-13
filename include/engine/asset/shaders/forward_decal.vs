#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;


uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;

void main()
{
    TexCoords = aTexCoord;
    Normal = normalize(mat3(transpose(inverse(u_Model))) * vec3(0.0, 0.0, 1.0));
    vec3 localPos = vec3(aPos.x, aPos.y, 0.01);
    vec4 worldPos = u_Model * vec4(localPos, 1.0);
    FragPos = worldPos.xyz;
    gl_Position = u_Projection * u_View * worldPos;
}
