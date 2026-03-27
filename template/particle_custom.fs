#version 430 core
out vec4 FragColor;

in vec2 TexCoords;
in vec4 ParticleColor;

uniform sampler2D u_AlbedoMap;

void main()
{
    vec4 texColor = texture(u_AlbedoMap, TexCoords);
    
    // CUSTOM LOGIC HERE (e.g., color tinting, alpha modulation)
    vec4 finalColor = texColor * ParticleColor;
    
    FragColor = finalColor;
}
