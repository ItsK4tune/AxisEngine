#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_AlbedoMap;
uniform vec4 u_TintColor;

uniform vec2 u_UVScale = vec2(1.0, 1.0);
uniform vec2 u_UVOffset = vec2(0.0, 0.0);

uniform bool debug_noTexture;

struct Material {
    float u_Opacity;
};
uniform Material material;

void main()
{    
    if (debug_noTexture) {
         FragColor = vec4(1.0, 1.0, 1.0, material.u_Opacity) * u_TintColor;
    } else {
         vec4 texColor = texture(u_AlbedoMap, TexCoords * u_UVScale + u_UVOffset);
         FragColor = vec4(texColor.rgb, texColor.a * material.u_Opacity) * u_TintColor;
    }
}

