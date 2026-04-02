#version 430 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;

void main()
{
    float dist = 0.01;
    vec3 color;
    color.r = texture(screenTexture, TexCoords + vec2(dist, 0)).r;
    color.g = texture(screenTexture, TexCoords).g;
    color.b = texture(screenTexture, TexCoords - vec2(dist, 0)).b;
    FragColor = vec4(color, 1.0);
}

