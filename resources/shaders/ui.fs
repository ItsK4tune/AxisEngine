#version 330 core

in vec2 TexCoords;

uniform vec4 spriteColor;

out vec4 color;

void main() {
    color = spriteColor;
}