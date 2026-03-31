#version 430 core
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;

float dither(vec2 pos) {
    int x = int(mod(pos.x, 4.0));
    int y = int(mod(pos.y, 4.0));
    int index = x + y * 4;
    float threshold[16] = float[](
        0.0625, 0.5625, 0.1875, 0.6875,
        0.8125, 0.3125, 0.9375, 0.4375,
        0.25, 0.75, 0.125, 0.625,
        1.0, 0.5, 0.875, 0.375
    );
    return threshold[index];
}

void main()
{
    vec3 color = texture(screenTexture, TexCoords).rgb;
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    
    // Scale gray to some levels if needed, or just B/W
    float limit = dither(gl_FragCoord.xy);
    float bw = gray > limit ? 1.0 : 0.0;
    
    FragColor = vec4(vec3(bw), 1.0);
}
