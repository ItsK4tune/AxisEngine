#version 430 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;

uniform sampler2D decalAlbedo;
uniform float opacity;
uniform vec4 tintColor;

// Simplified lighting data
layout(std140, binding = 21) uniform LightData {
    mat4 lightSpaceMatricesDir[2];
    mat4 lightSpaceMatricesSpot[2];
    int numDirLights;
    int nrPointLights;
    int nrSpotLights;
    int u_ReceiveShadow;
    float farPlanePoint;
    float farPlaneSpot;
} light;

struct DirLight {
    vec3 direction; float shadowIndex;
    vec3 color; float intensity;
    vec3 ambient; float pad1;
    vec3 diffuse; float pad2;
    vec3 specular; float pad3;
};

layout(std430, binding = 23) buffer DirLightBuffer { DirLight dirLights[]; };

void main()
{
    vec4 texColor = texture(decalAlbedo, TexCoords);
    if (texColor.a < 0.01) discard;

    // Linearize
    vec3 albedo = pow(texColor.rgb, vec3(2.2)) * tintColor.rgb;
    vec3 N = normalize(Normal);
    vec3 Lo = vec3(0.0);
    
    for(int i = 0; i < light.numDirLights; i++) {
        vec3 L = normalize(-dirLights[i].direction);
        float diff = max(dot(N, L), 0.0);
        vec3 radiance = dirLights[i].color * dirLights[i].intensity;
        Lo += (0.1 * radiance + 0.8 * diff * radiance);
    }

    FragColor = vec4(albedo * Lo, texColor.a * opacity * tintColor.a);
}
