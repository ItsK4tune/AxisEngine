#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_Mvp;
uniform mat4 u_Model;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;

void main()
{
    gl_Position = vec4(a_Position, 1.0) * u_Mvp;
    v_FragPos = (vec4(a_Position, 1.0) * u_Model).xyz;
    v_Normal = normalize(a_Normal * mat3(u_Model));
    v_TexCoord = a_TexCoord;
}
