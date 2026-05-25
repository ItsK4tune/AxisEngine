#version 430 core
out vec4 FragColor;

layout (binding = 0) uniform sampler2D u_AlbedoMap;
layout (binding = 1) uniform sampler2D u_NormalMap;
layout (binding = 2) uniform sampler2D u_MetallicMap;
layout (binding = 3) uniform sampler2D u_RoughnessMap;
layout (binding = 4) uniform sampler2D u_AOMap;
layout (binding = 5) uniform sampler2D u_EmissiveMap;

layout (binding = 6) uniform samplerCube u_IrradianceMap;
layout (binding = 7) uniform samplerCube u_PrefilterMap;
layout (binding = 8) uniform sampler2D u_BrdfLUT;

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
uniform bool debug_noTexture;

uniform float u_Reflectivity;
uniform float u_ReflectionIntensity;
uniform float u_FresnelPower;
uniform float u_FresnelBias;
uniform bool u_HasProbe;
uniform vec3 u_ProbePos;
uniform vec3 u_ProbeBoxMin;
uniform vec3 u_ProbeBoxMax;
layout (binding = 15) uniform samplerCube u_ReflectionProbe;

vec3 BoxProjection(vec3 dir, vec3 pos, vec3 probePos, vec3 boxMin, vec3 boxMax) {
    vec3 rbmax = (boxMax - pos) / dir;
    vec3 rbmin = (boxMin - pos) / dir;
    vec3 rbminmax = mix(rbmax, rbmin, step(vec3(0.0), dir));
    float fa = min(min(rbminmax.x, rbminmax.y), rbminmax.z);
    vec3 posonbox = pos + dir * fa;
    return posonbox - probePos;
}

uniform vec3 u_SH[9];
uniform bool u_HasLightProbe = false;
uniform float u_LightProbeIntensity = 1.0;

vec3 EvaluateSH(vec3 n) {
    const float C1 = 0.429043;
    const float C2 = 0.511664;
    const float C3 = 0.743125;
    const float C4 = 0.886227;
    const float C5 = 0.247708;
    return max(vec3(0.0), (
        C4 * u_SH[0] -
        C2 * n.y * u_SH[1] +
        C2 * n.z * u_SH[2] -
        C2 * n.x * u_SH[3] +
        C1 * n.y * n.x * u_SH[4] -
        C1 * n.y * n.z * u_SH[5] +
        C3 * (n.z * n.z - 1.0/3.0) * u_SH[6] -
        C1 * n.z * n.x * u_SH[7] +
        C5 * (n.x * n.x - n.y * n.y) * u_SH[8]
    ));
}

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float u_Roughness);
float GeometrySchlickGGX(float NdotV, float u_Roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float u_Roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float u_Roughness);

void main()
{
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;

    if (debug_noTexture) {
        albedo = u_BaseColor.rgb;
        metallic = u_Metallic;
        roughness = u_Roughness;
        ao = u_AO;
    } else {
        albedo = pow(texture(u_AlbedoMap, TexCoords).rgb, vec3(2.2)) * u_BaseColor.rgb;
        metallic = texture(u_MetallicMap, TexCoords).r * u_Metallic;
        roughness = texture(u_RoughnessMap, TexCoords).r * u_Roughness;
        ao = texture(u_AOMap, TexCoords).r * u_AO;
    }
    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.04, 1.0);

    vec3 N = normalize(Normal);
    vec3 V = normalize(camera.viewPos.xyz - FragPos);
    vec3 R = reflect(-V, N);

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

    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    
    vec3 irradiance;
    if (u_HasLightProbe) {
        irradiance = EvaluateSH(N) * u_LightProbeIntensity;
    } else {
        irradiance = texture(u_IrradianceMap, N).rgb;
    }
    vec3 diffuse = irradiance * albedo;
    
    vec3 prefilteredColor;
    if (u_HasProbe && u_Reflectivity > 0.0) {
        vec3 projectedR = BoxProjection(R, FragPos, u_ProbePos, u_ProbeBoxMin, u_ProbeBoxMax);
        prefilteredColor = textureLod(u_ReflectionProbe, projectedR, roughness * 5.0).rgb;
    } else {
        prefilteredColor = textureLod(u_PrefilterMap, R, roughness * 4.0).rgb;
    }

    vec2 envBRDF = texture(u_BrdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 u_Specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    vec3 u_Ambient = (kD * diffuse + u_Specular) * ao;
    vec3 emissive = u_Emission + pow(texture(u_EmissiveMap, TexCoords).rgb, vec3(2.2));
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
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float u_Roughness) {
    return F0 + (max(vec3(1.0 - u_Roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}




