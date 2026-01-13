#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec4 ParticleColor;

uniform sampler2D sprite; // texture

void main()
{
    // Debug UVs
    // FragColor = vec4(TexCoords, 0.0, 1.0); return; 
    FragColor = texture(sprite, TexCoords) * ParticleColor;
    if(FragColor.a < 0.1)
        discard;
}
