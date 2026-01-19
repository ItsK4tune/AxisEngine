#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec4 tintColor;

uniform bool debug_noTexture;

void main()
{    
    if (debug_noTexture) {
         FragColor = vec4(1.0) * tintColor;
    } else {
         FragColor = texture(texture_diffuse1, TexCoords) * tintColor;
    }
}
