#version 460 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;
layout (location = 4) out vec3 gEmissive;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

uniform uint entityID;


layout(std140, binding = 20) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec3 viewPos;
} camera;


layout (binding = 27) uniform sampler2D splatMap;
layout (binding = 28) uniform sampler2D textureLayer0;
layout (binding = 29) uniform sampler2D textureLayer1;
layout (binding = 30) uniform sampler2D textureLayer2;
layout (binding = 31) uniform sampler2D textureLayer3;

uniform float textureScale;
uniform bool debug_noTexture;

void main()
{
    gPosition = WorldPos;
    gNormal = normalize(Normal) * 0.5 + 0.5;
    
    vec3 albedo;
    if (debug_noTexture) {
        albedo = vec3(1.0);
    } else {
        vec4 splat = texture(splatMap, TexCoords);
        vec2 tiledCoords = TexCoords * textureScale;
        
        vec3 col0 = pow(texture(textureLayer0, tiledCoords).rgb, vec3(2.2));
        vec3 col1 = pow(texture(textureLayer1, tiledCoords).rgb, vec3(2.2));
        vec3 col2 = pow(texture(textureLayer2, tiledCoords).rgb, vec3(2.2));
        vec3 col3 = pow(texture(textureLayer3, tiledCoords).rgb, vec3(2.2));
        
        albedo = col0 * splat.r + 
                 col1 * splat.g + 
                 col2 * splat.b + 
                 col3 * (1.0 - clamp(splat.r + splat.g + splat.b, 0.0, 1.0));
    }
    
    gAlbedoSpec.rgb = albedo;
    gAlbedoSpec.a = 0.5;
    gEntityID = entityID;
    gEmissive = vec3(0.0);
}
