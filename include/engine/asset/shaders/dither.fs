#version 430 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

float Bayer4x4(vec2 p)
{
    int x = int(mod(p.x, 4.0));
    int y = int(mod(p.y, 4.0));
    int index = x + y * 4;
    int matrix[16] = int[](
        0, 8, 2, 10,
        12, 4, 14, 6,
        3, 11, 1, 9,
        15, 7, 13, 5
    );
    return (float(matrix[index]) + 0.5) / 16.0;
}

void main()
{
    vec3 color = texture(screenTexture, TexCoords).rgb;
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    float threshold = Bayer4x4(gl_FragCoord.xy);
    float dithered = gray > threshold ? 1.0 : 0.0;
    FragColor = vec4(vec3(dithered), 1.0);
}
