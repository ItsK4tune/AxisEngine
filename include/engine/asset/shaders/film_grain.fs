#version 430 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;

float random(vec2 uv) {
    return fract(sin(dot(uv.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main()
{
    vec3 color = texture(screenTexture, TexCoords).rgb;
    float grain = random(TexCoords + 0.1) * 0.1;
    FragColor = vec4(color + grain, 1.0);
}

