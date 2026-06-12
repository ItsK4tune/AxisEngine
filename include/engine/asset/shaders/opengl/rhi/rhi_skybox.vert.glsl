#version 460 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_Mvp;

out vec3 v_TexCoord;

void main()
{
    vec4 pos = vec4(a_Position, 1.0) * u_Mvp;
    gl_Position = pos.xyww;
    v_TexCoord = a_Position;
}
