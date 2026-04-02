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

layout (binding = 0) uniform sampler2D u_AlbedoMap;
layout (binding = 1) uniform sampler2D u_NormalMap;
layout (binding = 2) uniform sampler2D u_MetallicMap;
layout (binding = 3) uniform sampler2D u_RoughnessMap;
layout (binding = 4) uniform sampler2D u_AOMap;
layout (binding = 5) uniform sampler2D u_EmissiveMap;

layout(std140, binding = 20) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec4 viewPos;
    mat4 invProjection;
    mat4 invView;
    mat4 stableProjection;
    mat4 invStableProjection;
} camera;

uniform float u_Roughness;
uniform float u_Metallic;
uniform float u_AO;
uniform vec3 u_Emission;
uniform vec4 u_BaseColor; // rgb: tint, a: opacity
uniform bool debug_noTexture;
uniform uint entityID;
uniform bool u_isWireframe;
uniform bool u_ProbeUnlit;
uniform float u_Reflectivity;
uniform float u_FresnelPower;

void main()
{    
    gPosition = FragPos;
    gNormal = normalize(Normal);
    
    vec4 texColor;
    float _roughness;
    if (debug_noTexture) {
        texColor = vec4(1.0);
        _roughness = u_Roughness;
    } else {
        texColor = texture(u_AlbedoMap, TexCoords);
        texColor.rgb = pow(texColor.rgb, vec3(2.2));
        _roughness = texture(u_RoughnessMap, TexCoords).r * u_Roughness;
    }
    
    gAlbedoSpec.rgb = texColor.rgb * u_BaseColor.rgb;
    gAlbedoSpec.a = _roughness; // Pack roughness into alpha
    gEntityID = entityID;
    gEmissive = u_Emission + texture(u_EmissiveMap, TexCoords).rgb;
    
    // R: Metallic, G: Roughness, B: Reflectivity, A: FresnelPower
    gPBRParams = vec4(u_Metallic, _roughness, u_Reflectivity, u_FresnelPower);

    if (u_isWireframe) {
        gAlbedoSpec.rgb = vec3(0.0, 1.0, 0.0);
        gEmissive = vec3(0.0);
    }
}
