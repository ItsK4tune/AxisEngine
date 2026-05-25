#version 430 core
out vec4 FragColor;

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

layout(binding = 10) uniform sampler2DArray u_ShadowMapDir;
layout(binding = 11) uniform samplerCubeArray u_ShadowMapPoint;
layout(binding = 12) uniform sampler2DArray u_ShadowMapSpot;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform float u_Roughness;
uniform float u_Metallic;
uniform float u_AO;
uniform vec3 u_Emission;
uniform vec4 u_BaseColor;
uniform vec2 u_UVScale = vec2(1.0);
uniform vec2 u_UVOffset = vec2(0.0);
uniform bool debug_noTexture;
uniform bool u_IsWireframe;

uniform float u_ShadowBias = 0.005;
uniform int u_ShadowSoftness = 1;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float u_Roughness);
float GeometrySchlickGGX(float NdotV, float u_Roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float u_Roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
float ShadowCalculationDir(vec4 fragPosLightSpace, int lightIdx);
float ShadowCalculationPoint(vec3 fragPos, vec3 u_LightPos, int lightIdx);
float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int lightIdx);

void main()
{
    vec2 finalTexCoords = TexCoords * u_UVScale + u_UVOffset;
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
        albedo = pow(texture(u_AlbedoMap, finalTexCoords).rgb, vec3(2.2)) * u_BaseColor.rgb;
        metallic = texture(u_MetallicMap, finalTexCoords).r * u_Metallic;
        roughness = texture(u_RoughnessMap, finalTexCoords).r * u_Roughness;
        ao = texture(u_AOMap, finalTexCoords).r * u_AO;
    }
    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.04, 1.0);

    vec3 N = normalize(Normal);
    vec3 V = normalize(camera.viewPos.xyz - FragPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    
    for(int d = 0; d < light.numDirLights; d++) {
        vec3 L = normalize(-dirLights[d].direction);
        vec3 H = normalize(V + L);
        float shadow = 0.0;
        int sIdx = int(dirLights[d].shadowIndex);
        if (light.u_ReceiveShadow != 0 && sIdx >= 0 && sIdx < 16) {
            vec4 fragPosLightSpace = light.lightSpaceMatricesDir[sIdx] * vec4(FragPos, 1.0);
            shadow = ShadowCalculationDir(fragPosLightSpace, sIdx);
        }

        if(shadow < 1.0) {
            vec3 radiance = dirLights[d].color * dirLights[d].intensity;
            float NDF = DistributionGGX(N, H, roughness);
            float G   = GeometrySmith(N, V, L, roughness);
            vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 u_Specular = numerator / denominator;
            vec3 kS = F;
            vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
            Lo += (kD * albedo / PI + u_Specular) * radiance * max(dot(N, L), 0.0) * (1.0 - shadow);
        }
    }

    for(int i = 0; i < light.nrPointLights; ++i) {
        vec3 L = normalize(pointLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float dist = length(pointLights[i].position - FragPos);
        float atten = 1.0 / (pointLights[i].constant + pointLights[i].linear * dist + pointLights[i].quadratic * (dist * dist));
        
        float shadow = 0.0;
        int sIdx = int(pointLights[i].shadowIndex);
        if (light.u_ReceiveShadow != 0 && sIdx >= 0 && sIdx < 16) {
            shadow = ShadowCalculationPoint(FragPos, pointLights[i].position, sIdx);
        }

        vec3 radiance = pointLights[i].color * pointLights[i].intensity * atten;
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 u_Specular = (NDF * G * F) / (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        Lo += (kD * albedo / PI + u_Specular) * radiance * max(dot(N, L), 0.0) * (1.0 - shadow);
    }

    for(int i = 0; i < light.nrSpotLights; ++i) {
        vec3 L = normalize(spotLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float dist = length(spotLights[i].position - FragPos);
        float atten = 1.0 / (spotLights[i].constant + spotLights[i].linear * dist + spotLights[i].quadratic * (dist * dist));
        
        float theta = dot(L, normalize(-spotLights[i].direction));
        float epsilon = spotLights[i].cutOff - spotLights[i].outerCutOff;
        float spotIntensity = clamp((theta - spotLights[i].outerCutOff) / epsilon, 0.0, 1.0);

        float shadow = 0.0;
        int sIdx = int(spotLights[i].shadowIndex);
        if (light.u_ReceiveShadow != 0 && sIdx >= 0 && sIdx < 16) {
            vec4 fragPosLightSpace = light.lightSpaceMatricesSpot[sIdx] * vec4(FragPos, 1.0);
            shadow = ShadowCalculationSpot(fragPosLightSpace, N, L, sIdx);
        }

        vec3 radiance = spotLights[i].color * spotLights[i].intensity * atten * spotIntensity;
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 u_Specular = (NDF * G * F) / (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        Lo += (kD * albedo / PI + u_Specular) * radiance * max(dot(N, L), 0.0) * (1.0 - shadow);
    }

    vec3 u_Ambient = albedo * 0.03 * ao;
    vec3 emissive = u_Emission + pow(texture(u_EmissiveMap, finalTexCoords).rgb, vec3(2.2));
    
    vec3 finalColor = u_Ambient + Lo + emissive;
    FragColor = vec4(finalColor, u_BaseColor.a);

    if (u_IsWireframe) {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
    }
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

float ShadowCalculationDir(vec4 fragPosLightSpace, int lightIdx) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    float currentDepth = projCoords.z;
    float bias = u_ShadowBias;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMapDir, 0).xy;
    int pcfRange = u_ShadowSoftness;
    for(int x = -pcfRange; x <= pcfRange; ++x) {
        for(int y = -pcfRange; y <= pcfRange; ++y) {
            float pcfDepth = texture(u_ShadowMapDir, vec3(projCoords.xy + vec2(x, y) * texelSize, lightIdx)).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / pow(pcfRange * 2 + 1, 2);
}

float ShadowCalculationPoint(vec3 fragPos, vec3 u_LightPos, int lightIdx) {
    vec3 fragToLight = fragPos - u_LightPos;
    float currentDepth = length(fragToLight);
    float bias = u_ShadowBias * 10.0;
    float shadow = 0.0;
    int samples = 20;
    vec3 sampleOffsetDirections[20] = vec3[] (
       vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
       vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
       vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
       vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
       vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );
    float diskRadius = (1.0 + (currentDepth / light.farPlanePoint)) / 25.0;
    for(int i = 0; i < samples; ++i) {
        float closestDepth = texture(u_ShadowMapPoint, vec4(fragToLight + sampleOffsetDirections[i] * diskRadius, lightIdx)).r;
        closestDepth *= light.farPlanePoint;
        if(currentDepth - bias > closestDepth) shadow += 1.0;
    }
    return shadow / float(samples);
}

float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int lightIdx) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w * 0.5 + 0.5;
    if(projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    if(projCoords.z > 1.0) return 0.0;
    float currentDepth = projCoords.z;
    float ndotl = max(dot(normalize(normal), normalize(lightDir)), 0.0);
    float bias = max(u_ShadowBias * 0.25 * (1.0 - ndotl), 0.00005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMapSpot, 0).xy;
    int pcfRange = u_ShadowSoftness;
    for(int x = -pcfRange; x <= pcfRange; ++x) {
        for(int y = -pcfRange; y <= pcfRange; ++y) {
            float pcfDepth = texture(u_ShadowMapSpot, vec3(projCoords.xy + vec2(x, y) * texelSize, lightIdx)).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / pow(pcfRange * 2 + 1, 2);
}




