#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_metallic1;
    sampler2D texture_roughness1;
    sampler2D texture_ao1;
    sampler2D texture_normal1;
    sampler2D texture_emission1;

    float roughness;
    float metallic;
    float ao;
    vec3 emission;
};

uniform Material material;
uniform vec4 tintColor;
uniform bool debug_noTexture;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;
    
    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);
    
    // and the diffuse per-fragment color, and store specular intensity in gAlbedoSpec's alpha component
    if (debug_noTexture)
    {
        gAlbedoSpec.rgb = tintColor.rgb;
        gAlbedoSpec.a = material.roughness;
    }
    else
    {
        gAlbedoSpec.rgb = texture(material.texture_diffuse1, TexCoords).rgb * tintColor.rgb;
        gAlbedoSpec.a = texture(material.texture_roughness1, TexCoords).r * material.roughness;
    }
}
