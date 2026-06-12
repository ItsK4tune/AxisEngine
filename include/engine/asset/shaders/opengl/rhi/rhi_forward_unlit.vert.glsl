#version 460 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_Mvp;
uniform vec4 u_Color;

out vec4 v_Color;

void main()
{
    gl_Position = vec4(a_Position, 1.0) * u_Mvp;
    v_Color = u_Color;
}
