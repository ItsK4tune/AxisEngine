#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec4 tintColor;

uniform vec2 uvScale = vec2(1.0, 1.0);
uniform vec2 uvOffset = vec2(0.0, 0.0);

uniform bool debug_noTexture;

void main()
{    
    if (debug_noTexture) {
         FragColor = vec4(1.0) * tintColor;
    } else {
         FragColor = texture(texture_diffuse1, TexCoords * uvScale + uvOffset) * tintColor;
    }
}
