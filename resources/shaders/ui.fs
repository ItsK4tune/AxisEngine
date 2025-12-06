#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform vec4 spriteColor;
uniform sampler2D image;
uniform bool hasTexture;

void main()
{
    if(hasTexture) {
        color = texture(image, TexCoords) * spriteColor;
    } else {
        color = spriteColor;
    }
}