#version 460 core
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;
layout (location = 4) out vec3 gEmissive;
layout (location = 5) out vec4 gPBRParams;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

layout (binding = 0) uniform sampler2D u_AlbedoMap;
layout (binding = 2) uniform sampler2D u_MetallicMap;
layout (binding = 3) uniform sampler2D u_RoughnessMap;
layout (binding = 4) uniform sampler2D u_AOMap;
layout (binding = 5) uniform sampler2D u_EmissiveMap;

uniform float u_Roughness;
uniform float u_Metallic;
uniform vec3 u_Emission;
uniform vec4 u_BaseColor;
uniform vec2 u_UVScale = vec2(1.0);
uniform vec2 u_UVOffset = vec2(0.0);
flat in uint EntityID;
uniform float u_Reflectivity;
uniform float u_FresnelPower;
uniform float u_FresnelBias;
uniform float u_ReceiveShadowFlag = 1.0;

void main()
{
    vec2 uv = TexCoords * u_UVScale + u_UVOffset;

    vec4 texColor = texture(u_AlbedoMap, uv);
    texColor.rgb = pow(texColor.rgb, vec3(2.2));
    float metallic = clamp(u_Metallic * texture(u_MetallicMap, uv).r, 0.0, 1.0);
    float roughness = clamp(u_Roughness * texture(u_RoughnessMap, uv).r, 0.04, 1.0);

    gNormal = vec4(normalize(Normal), u_ReceiveShadowFlag);
    gAlbedoSpec.rgb = texColor.rgb * u_BaseColor.rgb;
    gAlbedoSpec.a = max(u_FresnelBias, 0.04);
    gEntityID = EntityID;
    gEmissive = u_Emission + texture(u_EmissiveMap, uv).rgb;

    float packedReflection = round(clamp(u_FresnelPower, 0.0, 15.0)) / 255.0;
    gPBRParams = vec4(metallic, roughness, u_Reflectivity, packedReflection);
}
