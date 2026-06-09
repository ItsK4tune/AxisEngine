#version 460 core

in vec2 TexCoords;

out vec4 color;



uniform vec4 u_SpriteColor;

uniform sampler2D image;

uniform bool u_HasTexture;



void main()

{

    if(u_HasTexture) {

        color = texture(image, TexCoords) * u_SpriteColor;

    } else {

        color = u_SpriteColor;

    }

}


