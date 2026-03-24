#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec4 tintColor;

uniform bool debug_noTexture;

struct Material {
    float opacity;
};
uniform Material material;
uniform bool u_isWireframe;

void main()
{    
    if (u_isWireframe) {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
        return;
    }
    if (debug_noTexture) {
         FragColor = vec4(1.0, 1.0, 1.0, material.opacity) * tintColor;
    } else {
         vec4 texColor = texture(texture_diffuse1, TexCoords);
         texColor.rgb = pow(texColor.rgb, vec3(2.2));
         FragColor = vec4(texColor.rgb, texColor.a * material.opacity) * tintColor;
    }
}
