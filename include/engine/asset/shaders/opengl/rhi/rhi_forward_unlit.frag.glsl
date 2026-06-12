#version 460 core

in vec4 v_Color;

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
    vec3 color = ACESFilm(v_Color.rgb);
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, v_Color.a);
}
