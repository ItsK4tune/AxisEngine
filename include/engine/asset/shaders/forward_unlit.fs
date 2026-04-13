#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_AlbedoMap;
uniform vec4 u_BaseColor;

uniform bool debug_noTexture;
uniform bool u_IsWireframe;

void main()
{    
    if (u_IsWireframe) {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
        return;
    }
    if (debug_noTexture) {
         FragColor = u_BaseColor;
    } else {
         vec4 texColor = texture(u_AlbedoMap, TexCoords);
         texColor.rgb = pow(texColor.rgb, vec3(2.2));
         FragColor = vec4(texColor.rgb, texColor.a) * u_BaseColor;
    }
}

