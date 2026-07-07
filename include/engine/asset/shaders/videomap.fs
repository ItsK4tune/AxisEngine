#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_AlbedoMap;
uniform vec4 u_TintColor;
uniform vec4 u_BaseColor = vec4(1.0);
uniform vec2 u_UVScale = vec2(1.0, 1.0);
uniform vec2 u_UVOffset = vec2(0.0, 0.0);

void main()
{
    vec4 texColor = texture(u_AlbedoMap, TexCoords * u_UVScale + u_UVOffset);
    FragColor = vec4(texColor.rgb, texColor.a * u_BaseColor.a) * u_TintColor;
}
