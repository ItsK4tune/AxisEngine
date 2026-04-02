#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;
layout (location = 4) out vec3 gEmissive;
layout (location = 5) out vec4 gPBRParams;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform vec3 teamColor;
uniform vec4 tintColor;
uniform uint entityID;

void main()
{    
    // 1. Position and Normal
    gPosition = FragPos;
    gNormal = normalize(Normal);
    
    // 2. Albedo and Emissive logic
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    vec3 result = texColor.rgb * tintColor.rgb;
    
    // Sample texture alpha or specific mask to apply teamColor
    if (texColor.a < 0.1) {
        result = teamColor;
    }

    gAlbedoSpec = vec4(result, 1.0); // Roughness 1.0
    gEntityID = entityID;
    gEmissive = result * 0.2; // Small emissive glow
    
    // R: Metallic, G: Roughness, B: Reflectivity, A: FresnelPower
    gPBRParams = vec4(0.0, 1.0, 0.0, 0.0); 
}
