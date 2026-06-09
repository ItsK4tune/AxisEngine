#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;



void main()

{

    vec2 uv = TexCoords - 0.5;

    float dist = length(uv);

    float vignette = smoothstep(0.8, 0.4, dist);

    vec3 color = texture(screenTexture, TexCoords).rgb;

    FragColor = vec4(color * vignette, 1.0);

}


