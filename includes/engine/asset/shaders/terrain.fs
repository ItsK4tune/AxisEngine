#version 460 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

uniform sampler2D splatMap;
uniform sampler2D textureLayer0; // Grass
uniform sampler2D textureLayer1; // Dirt
uniform sampler2D textureLayer2; // Rock
uniform sampler2D textureLayer3; // Snow

uniform float textureScale;
uniform vec3 camPos;

// PBR uniforms (simplified for now)
uniform vec3 lightDir;
uniform vec3 lightColor;

void main()
{
    // Read splat map
    vec4 splat = texture(splatMap, TexCoords);
    
    // Scale texture coordinates for tiled textures
    vec2 tiledCoords = TexCoords * textureScale;
    
    // Sample layers
    vec4 col0 = texture(textureLayer0, tiledCoords);
    vec4 col1 = texture(textureLayer1, tiledCoords);
    vec4 col2 = texture(textureLayer2, tiledCoords);
    vec4 col3 = texture(textureLayer3, tiledCoords);
    
    // Mix layers based on splat map
    vec3 albedo = col0.rgb * splat.r + 
                  col1.rgb * splat.g + 
                  col2.rgb * splat.b + 
                  col3.rgb * (1.0 - (splat.r + splat.g + splat.b));
    
    // Simple Lighting (PBR-lite for now)
    vec3 N = normalize(Normal);
    vec3 L = normalize(-lightDir);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 ambient = vec3(0.05) * albedo;
    
    FragColor = vec4(ambient + diffuse * albedo, 1.0);
}
