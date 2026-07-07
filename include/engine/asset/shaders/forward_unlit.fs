#version 460 core

out vec4 FragColor;



in vec2 TexCoords;



uniform sampler2D u_AlbedoMap;

uniform vec4 u_BaseColor;



void main()

{    

     vec4 texColor = texture(u_AlbedoMap, TexCoords);

     texColor.rgb = pow(texColor.rgb, vec3(2.2));

     FragColor = vec4(texColor.rgb, texColor.a) * u_BaseColor;

}
