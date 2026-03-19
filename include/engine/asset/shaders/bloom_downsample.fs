#version 330 core

// This shader performs downsampling by taking 13 samples in a specific pattern
// to ensure a smooth, stable bloom effect.

uniform sampler2D srcTexture;
uniform vec2 srcResolution;
uniform float threshold;

in vec2 TexCoords;
layout (location = 0) out vec3 downsample;

vec3 KarisAverage(vec3 color) {
    float res = 1.0 / (1.0 + max(max(color.r, color.g), color.b));
    return color * res;
}

void main()
{
    vec2 srcTexelSize = 1.0 / srcResolution;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    // Take 13 samples around current texel:
    // a - b - c
    // - d - e -
    // f - g - h
    // - i - j -
    // k - l - m

    // Center
    vec3 e = texture(srcTexture, vec2(TexCoords.x, TexCoords.y)).rgb;
    // Inner
    vec3 d = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;
    vec3 f = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;
    vec3 i = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;
    vec3 j = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;
    // Outer
    vec3 a = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y + 2*y)).rgb;
    vec3 b = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y + 2*y)).rgb;
    vec3 c = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y + 2*y)).rgb;
    vec3 g = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y)).rgb;
    vec3 h = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y)).rgb;
    vec3 k = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y - 2*y)).rgb;
    vec3 l = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y - 2*y)).rgb;
    vec3 m = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y - 2*y)).rgb;

    // Weighting:
    // e: 0.125
    // d, f, i, j: 0.125 * 4 = 0.5
    // a, c, k, m: 0.03125 * 4 = 0.125
    // b, g, h, l: 0.0625 * 4 = 0.25
    // Total: 0.125 + 0.5 + 0.125 + 0.25 = 1.0

    downsample = e*0.125;
    downsample += (d+f+i+j)*0.125;
    downsample += (a+c+k+m)*0.03125;
    downsample += (b+g+h+l)*0.0625;

    // Apply threshold
    float brightness = max(max(downsample.r, downsample.g), downsample.b);
    if (brightness < threshold) {
        downsample = vec3(0.0);
    }
}
