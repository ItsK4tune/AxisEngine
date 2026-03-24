#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;
uniform float exposure;
uniform float bloomIntensity;
uniform float gamma;
uniform int tonemappingMode;

vec3 ReinhardTonemap(vec3 color)
{
    return color / (color + vec3(1.0));
}

vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main()
{
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    

    hdrColor += bloomColor * bloomIntensity;


    vec3 result = hdrColor * exposure;


    if (tonemappingMode == 1) {
        result = ReinhardTonemap(result);
    } else if (tonemappingMode == 2) {
        result = ACESFilm(result);
    }


    result = pow(result, vec3(1.0 / gamma));

    FragColor = vec4(result, 1.0);
}
