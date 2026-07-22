#version 460 core
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;
layout (location = 4) out vec3 gEmissive;
layout (location = 5) out vec4 gPBRParams;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

uniform uint u_EntityID;

layout(std140, binding = 20) uniform CameraData {
    mat4 u_Projection;
    mat4 u_View;
    vec4 viewPos;
    mat4 u_InvProjection;
    mat4 u_InvView;
    mat4 stableProjection;
    mat4 invStableProjection;
} camera;

layout (binding = 27) uniform sampler2D splatMap;
layout (binding = 22) uniform sampler2D normalLayer0;
layout (binding = 23) uniform sampler2D normalLayer1;
layout (binding = 24) uniform sampler2D normalLayer2;
layout (binding = 25) uniform sampler2D normalLayer3;
layout (binding = 28) uniform sampler2D textureLayer0;
layout (binding = 29) uniform sampler2D textureLayer1;
layout (binding = 30) uniform sampler2D textureLayer2;
layout (binding = 31) uniform sampler2D textureLayer3;

uniform float textureScale;
uniform int normalLayerCount;
uniform int diffuseLayerCount;

void main()
{
    vec3 albedo;
    vec4 pbrParams = vec4(0.0, 0.85, 0.0, 0.0); // Metallic=0.0, Roughness=0.85, Reflectivity=0.0
    
    vec4 splat = texture(splatMap, TexCoords);
    vec2 tiledCoords = TexCoords * textureScale;
    
    vec3 col0 = diffuseLayerCount > 0 ? pow(texture(textureLayer0, tiledCoords).rgb, vec3(2.2)) : vec3(0.5);
    vec3 col1 = diffuseLayerCount > 1 ? pow(texture(textureLayer1, tiledCoords).rgb, vec3(2.2)) : col0;
    vec3 col2 = diffuseLayerCount > 2 ? pow(texture(textureLayer2, tiledCoords).rgb, vec3(2.2)) : col1;
    vec3 col3 = diffuseLayerCount > 3 ? pow(texture(textureLayer3, tiledCoords).rgb, vec3(2.2)) : col2;
    vec4 weights = vec4(splat.rgb, 1.0 - clamp(splat.r + splat.g + splat.b, 0.0, 1.0));

    vec3 worldNormal = normalize(Normal);
    if (normalLayerCount > 0)
    {
        vec3 tangentNormal = vec3(0.0);
        float normalWeight = 0.0;
        if (normalLayerCount > 0) { tangentNormal += (texture(normalLayer0, tiledCoords).xyz * 2.0 - 1.0) * weights.x; normalWeight += weights.x; }
        if (normalLayerCount > 1) { tangentNormal += (texture(normalLayer1, tiledCoords).xyz * 2.0 - 1.0) * weights.y; normalWeight += weights.y; }
        if (normalLayerCount > 2) { tangentNormal += (texture(normalLayer2, tiledCoords).xyz * 2.0 - 1.0) * weights.z; normalWeight += weights.z; }
        if (normalLayerCount > 3) { tangentNormal += (texture(normalLayer3, tiledCoords).xyz * 2.0 - 1.0) * weights.w; normalWeight += weights.w; }
        if (normalWeight > 0.0001)
        {
            tangentNormal = normalize(tangentNormal / normalWeight);
            vec3 dp1 = dFdx(WorldPos);
            vec3 dp2 = dFdy(WorldPos);
            vec2 duv1 = dFdx(tiledCoords);
            vec2 duv2 = dFdy(tiledCoords);
            vec3 tangent = normalize(dp1 * duv2.y - dp2 * duv1.y);
            vec3 bitangent = normalize(-dp1 * duv2.x + dp2 * duv1.x);
            worldNormal = normalize(mat3(tangent, bitangent, worldNormal) * tangentNormal);
        }
    }
    gNormal = vec4(worldNormal, 1.0); // alpha=1.0 enables receiving shadows
    
    albedo = col0 * splat.r + 
             col1 * splat.g + 
             col2 * splat.b + 
             col3 * weights.w;

    gAlbedoSpec.rgb = albedo;
    gAlbedoSpec.a = 0.04; // Default FresnelBias
    gEntityID = u_EntityID;
    gEmissive = vec3(0.0);
    gPBRParams = pbrParams;
}
