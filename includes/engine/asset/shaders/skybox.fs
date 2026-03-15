#version 430 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform float intensity;

void main()
{    
    FragColor = texture(skybox, TexCoords) * intensity;
}
