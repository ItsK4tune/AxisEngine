#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D decalAlbedo;
uniform float opacity;
uniform vec4 tintColor;

void main()
{
    vec4 texColor = texture(decalAlbedo, TexCoords);
    // Linearize
    texColor.rgb = pow(texColor.rgb, vec3(2.2));
    
    if (texColor.a < 0.01) discard;
    
    FragColor = vec4(texColor.rgb, texColor.a * opacity) * tintColor;
}
