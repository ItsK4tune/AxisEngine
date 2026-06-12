#version 460 core

in vec3 v_TexCoord;

uniform float u_Intensity;

layout(location = 0) out vec4 FragColor;

vec3 ACESFilm(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 dir = normalize(v_TexCoord);
    vec3 skyColor = mix(vec3(0.1, 0.2, 0.4), vec3(0.4, 0.6, 0.9),
                        clamp(dir.y * 0.5 + 0.5, 0.0, 1.0));
    vec3 color = ACESFilm(skyColor * u_Intensity);
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
}
