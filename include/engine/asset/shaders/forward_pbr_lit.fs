#version 460 core
out vec4 FragColor;

layout (binding = 0) uniform sampler2D u_AlbedoMap;
layout (binding = 1) uniform sampler2D u_NormalMap;
layout (binding = 2) uniform sampler2D u_MetallicMap;
layout (binding = 3) uniform sampler2D u_RoughnessMap;
layout (binding = 4) uniform sampler2D u_AOMap;
layout (binding = 5) uniform sampler2D u_EmissiveMap;
layout (binding = 30) uniform sampler2D u_Lightmap;
uniform float u_LightmapIntensity;

layout(std140, binding = 20) uniform CameraData {
    mat4 u_Projection;
    mat4 u_View;
    vec4 viewPos;
    mat4 u_InvProjection;
    mat4 u_InvView;
    mat4 stableProjection;
    mat4 invStableProjection;
} camera;

layout(std140, binding = 21) uniform LightData {
    mat4 lightSpaceMatricesDir[16];
    mat4 lightSpaceMatricesSpot[16];
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
    vec3 u_Ambient; float pad1;
    vec3 diffuse; float pad2;
    vec3 u_Specular; float pad3;
};

struct PointLight {
    vec3 position; float shadowIndex;
    vec3 color; float intensity;
    float constant; float linear; float quadratic; float radius;
    vec3 u_Ambient; float pad1;
    vec3 diffuse; float pad2;
    vec3 u_Specular; float pad3;
};

struct SpotLight {
    vec3 position; float pad0;
    vec3 direction; float shadowIndex;
    vec3 color; float intensity;
    float cutOff; float outerCutOff; float constant; float linear;
    float quadratic; float radius; float pad3; float pad4;
    vec3 u_Ambient; float pad5;
    vec3 diffuse; float pad6;
    vec3 u_Specular; float pad7;
};

layout(std430, binding = 23) buffer DirLightBuffer { DirLight dirLights[]; };
layout(std430, binding = 24) buffer PointLightBuffer { PointLight pointLights[]; };
layout(std430, binding = 25) buffer SpotLightBuffer { SpotLight spotLights[]; };

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform float u_Roughness;
uniform float u_Metallic;
uniform float u_AO;
uniform vec3 u_Emission;
uniform vec4 u_BaseColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float u_Roughness);
float GeometrySchlickGGX(float NdotV, float u_Roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float u_Roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

void main()
{
    vec3 albedo = pow(texture(u_AlbedoMap, TexCoords).rgb, vec3(2.2)) * u_BaseColor.rgb;
    float metallic = clamp(texture(u_MetallicMap, TexCoords).r * u_Metallic, 0.0, 1.0);
    float roughness = clamp(texture(u_RoughnessMap, TexCoords).r * u_Roughness, 0.04, 1.0);
    float ao = texture(u_AOMap, TexCoords).r * u_AO;

    vec3 N = normalize(Normal);
    vec3 V = normalize(camera.viewPos.xyz - FragPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    
    for(int d = 0; d < light.numDirLights; d++) {
        vec3 L = normalize(-dirLights[d].direction);
        vec3 H = normalize(V + L);
        vec3 radiance = dirLights[d].color * dirLights[d].intensity;
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 u_Specular = (NDF * G * F) / (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        Lo += (kD * albedo / PI + u_Specular) * radiance * max(dot(N, L), 0.0);
    }

    for(int i = 0; i < light.nrPointLights; ++i) {
        vec3 L = normalize(pointLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float dist = length(pointLights[i].position - FragPos);
        float atten = 1.0 / (pointLights[i].constant + pointLights[i].linear * dist + pointLights[i].quadratic * (dist * dist));
        vec3 radiance = pointLights[i].color * pointLights[i].intensity * atten;
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 u_Specular = (NDF * G * F) / (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        Lo += (kD * albedo / PI + u_Specular) * radiance * max(dot(N, L), 0.0);
    }

    vec3 u_Ambient = albedo * 0.03 * ao;
    vec3 emissive = u_Emission + pow(texture(u_EmissiveMap, TexCoords).rgb, vec3(2.2)) +
                    texture(u_Lightmap, TexCoords).rgb * u_LightmapIntensity;
    FragColor = vec4(u_Ambient + Lo + emissive, u_BaseColor.a);
}

float DistributionGGX(vec3 N, vec3 H, float u_Roughness) {
    float a = u_Roughness*u_Roughness; float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}
float GeometrySchlickGGX(float NdotV, float u_Roughness) {
    float r = (u_Roughness + 1.0); float k = (r*r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float u_Roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0), u_Roughness) * GeometrySchlickGGX(max(dot(N, L), 0.0), u_Roughness);
}
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}




