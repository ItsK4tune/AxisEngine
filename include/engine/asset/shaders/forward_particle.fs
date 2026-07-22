#version 460 core

out vec4 FragColor;



in vec2 TexCoords;

in vec4 ParticleColor;



uniform sampler2D u_AlbedoMap;



void main()

{

    vec4 texColor = texture(u_AlbedoMap, TexCoords);

    vec3 linearColor = pow(texColor.rgb, vec3(2.2));

    FragColor = vec4(linearColor, texColor.a) * ParticleColor;

}


