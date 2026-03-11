#version 330 core

// This shader performs upsampling using a 3x3 tent filter.

uniform sampler2D srcTexture;
uniform float filterRadius;

in vec2 TexCoords;
layout (location = 0) out vec3 upsample;

void main()
{
    // The filter kernel is applied to the source texture, which is a lower-resolution version.
    // The radius allows controlling the softness of the bloom.
    float x = filterRadius;
    float y = filterRadius;

    // a - b - c
    // d - e - f
    // g - h - i
    vec3 a = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;
    vec3 b = texture(srcTexture, vec2(TexCoords.x,     TexCoords.y + y)).rgb;
    vec3 c = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;

    vec3 d = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y)).rgb;
    vec3 e = texture(srcTexture, vec2(TexCoords.x,     TexCoords.y)).rgb;
    vec3 f = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y)).rgb;

    vec3 g = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;
    vec3 h = texture(srcTexture, vec2(TexCoords.x,     TexCoords.y - y)).rgb;
    vec3 i = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;

    // 1 2 1
    // 2 4 2
    // 1 2 1
    upsample = e*4.0;
    upsample += (b+d+f+h)*2.0;
    upsample += (a+c+g+i);
    upsample *= 1.0 / 16.0;
}
