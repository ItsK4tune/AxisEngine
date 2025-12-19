#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;

void main()
{
    vec4 col = texture(screenTexture, TexCoords);
    float gray = (col.r + col.g + col.b) / 3.0;
    FragColor = vec4(vec3(gray), 1.0);
}