#version 430 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform float intensity;

void main()
{    
    vec4 texColor = texture(skybox, TexCoords);
    vec3 linearColor = pow(texColor.rgb, vec3(2.2));
    FragColor = vec4(linearColor * intensity, texColor.a);
}
