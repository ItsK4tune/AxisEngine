#version 430 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D decalAlbedo;
uniform float opacity;
uniform vec4 tintColor;

void main()
{
    vec4 texColor = texture(decalAlbedo, TexCoords);
    if (texColor.a < 0.1) discard;

    vec3 albedo = pow(texColor.rgb, vec3(2.2)) * tintColor.rgb;
    
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
    float diff = max(dot(normalize(Normal), lightDir), 0.2);
    
    FragColor = vec4(albedo * diff, texColor.a * opacity * tintColor.a);
}

