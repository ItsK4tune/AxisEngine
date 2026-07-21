#version 460 core
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out uint gEntityID;
layout (location = 4) out vec3 gEmissive;
layout (location = 5) out vec4 gPBRParams;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform float u_Reflectivity = 1.0;
uniform float u_FresnelPower = 5.0;
uniform float u_FresnelBias = 0.04;
uniform int u_ProbeIndex = -1;

layout (binding = 0) uniform sampler2D u_AlbedoMap;
layout (binding = 1) uniform sampler2D u_NormalMap;
layout (binding = 2) uniform sampler2D u_MetallicMap;
layout (binding = 3) uniform sampler2D u_RoughnessMap;
layout (binding = 4) uniform sampler2D u_AOMap;
layout (binding = 5) uniform sampler2D u_EmissiveMap;

layout(std140, binding = 20) uniform CameraData {
    mat4 u_Projection;
    mat4 u_View;
    vec4 viewPos;
    mat4 u_InvProjection;
    mat4 u_InvView;
    mat4 stableProjection;
    mat4 invStableProjection;
} camera;

uniform float u_Roughness;
uniform float u_Metallic;
uniform float u_AO;
uniform vec3 u_Emission;
uniform vec4 u_BaseColor;
uniform uint u_EntityID;
flat in uint EntityID;

void main()
{    
    gNormal = Normal;
    
    vec4 texColor = texture(u_AlbedoMap, TexCoords);
    texColor.rgb = pow(texColor.rgb, vec3(2.2));
    float _roughness = texture(u_RoughnessMap, TexCoords).r * u_Roughness;
    
    gAlbedoSpec.rgb = texColor.rgb * u_BaseColor.rgb;
    gAlbedoSpec.a = u_FresnelBias; // Pack FresnelBias here (roughness already in gPBRParams.g)
    gEntityID = EntityID;
    gEmissive = u_Emission + texture(u_EmissiveMap, TexCoords).rgb;
    
    // Alpha is UNORM8: high nibble stores ProbeIndex+1, low nibble stores FresnelPower.
    float packedReflection = (clamp(float(u_ProbeIndex + 1), 0.0, 15.0) * 16.0 +
                              round(clamp(u_FresnelPower, 0.0, 15.0))) / 255.0;
    gPBRParams = vec4(u_Metallic, _roughness, u_Reflectivity, packedReflection);
}







