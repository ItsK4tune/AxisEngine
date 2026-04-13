#version 430 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;
layout (location = 4) out vec3 gEmissive;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;
uniform uint u_EntityID;

uniform sampler2D heightMap;
uniform sampler2D splatMap;
uniform sampler2D textureLayer0;
uniform sampler2D textureLayer1;
uniform sampler2D textureLayer2;
uniform sampler2D textureLayer3;

uniform float u_TextureScale;

void main()
{
    vec4 blend = texture(splatMap, TexCoords);
    vec2 scaledUV = TexCoords * u_TextureScale;
    
    vec4 c0 = texture(textureLayer0, scaledUV);
    vec4 c1 = texture(textureLayer1, scaledUV);
    vec4 c2 = texture(textureLayer2, scaledUV);
    vec4 c3 = texture(textureLayer3, scaledUV);
    
    vec3 finalColor = blend.r * c0.rgb + blend.g * c1.rgb + blend.b * c2.rgb + blend.a * c3.rgb;
    
    // CUSTOM LOGIC HERE
    
    gPosition = WorldPos;
    gNormal = normalize(Normal) * 0.5 + 0.5;
    gAlbedoSpec = vec4(finalColor, 1.0);
    gEmissive = vec3(0.0);
    gEntityID = u_EntityID;
}
