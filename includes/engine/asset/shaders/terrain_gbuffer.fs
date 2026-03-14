#version 460 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

uniform uint entityID;

layout (binding = 1) uniform sampler2D splatMap;
layout (binding = 2) uniform sampler2D textureLayer0; // Grass
layout (binding = 3) uniform sampler2D textureLayer1; // Dirt
layout (binding = 4) uniform sampler2D textureLayer2; // Rock
layout (binding = 5) uniform sampler2D textureLayer3; // Snow

uniform float textureScale;
uniform bool debug_noTexture;

void main()
{
    gPosition = WorldPos;
    gNormal = normalize(Normal) * 0.5 + 0.5; // Encode to [0, 1]
    
    vec3 albedo;
    if (debug_noTexture) {
        albedo = vec3(1.0);
    } else {
        vec4 splat = texture(splatMap, TexCoords);
        vec2 tiledCoords = TexCoords * textureScale;
        
        vec3 col0 = texture(textureLayer0, tiledCoords).rgb;
        vec3 col1 = texture(textureLayer1, tiledCoords).rgb;
        vec3 col2 = texture(textureLayer2, tiledCoords).rgb;
        vec3 col3 = texture(textureLayer3, tiledCoords).rgb;
        
        albedo = col0 * splat.r + 
                 col1 * splat.g + 
                 col2 * splat.b + 
                 col3 * (1.0 - (splat.r + splat.g + splat.b));
    }
    
    gAlbedoSpec.rgb = albedo;
    gAlbedoSpec.a = 0.5; // Default roughness
    gEntityID = entityID;
}
