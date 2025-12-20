#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec4 tintColor;

void main()
{
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    vec3 rgb = texColor.rgb * tintColor.rgb;
    float alpha = texColor.a * tintColor.a;
    FragColor = vec4(rgb, alpha);
}