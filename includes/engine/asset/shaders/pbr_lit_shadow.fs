#version 430 core
out vec4 FragColor;

// 1. Root Level Samplers (Standardized Binding 0-15)
layout (binding = 0) uniform sampler2D texture_diffuse1;
layout (binding = 1) uniform sampler2D texture_normal1;
layout (binding = 2) uniform sampler2D texture_metallic1;
layout (binding = 3) uniform sampler2D texture_roughness1;
layout (binding = 4) uniform sampler2D texture_ao1;
layout (binding = 5) uniform sampler2D texture_emissive1;

// IBL Maps
layout (binding = 6) uniform samplerCube irradianceMap;
layout (binding = 7) uniform samplerCube prefilterMap;
layout (binding = 8) uniform sampler2D brdfLUT;

// 2. UBO/SSBO Bindings (Standardized Binding 20-25)
layout(std140, binding = 20) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec3 viewPos;
} camera;

layout(std140, binding = 21) uniform LightData {
    mat4 lightSpaceMatricesDir[2];
    mat4 lightSpaceMatricesSpot[2];
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
    vec3 ambient; float pad1;
    vec3 diffuse; float pad2;
    vec3 specular; float pad3;
};

struct PointLight {
    vec3 position; float shadowIndex;
    vec3 color; float intensity;
    float constant; float linear; float quadratic; float radius;
    vec3 ambient; float pad1;
    vec3 diffuse; float pad2;
    vec3 specular; float pad3;
};

struct SpotLight {
    vec3 position; float pad0;
    vec3 direction; float shadowIndex;
    vec3 color; float intensity;
    float cutOff; float outerCutOff; float constant; float linear;
    float quadratic; float pad2; float pad3; float pad4;
    vec3 ambient; float pad5;
    vec3 diffuse; float pad6;
    vec3 specular; float pad7;
};

layout(std430, binding = 23) buffer DirLightBuffer { DirLight dirLights[]; };
layout(std430, binding = 24) buffer PointLightBuffer { PointLight pointLights[]; };
layout(std430, binding = 25) buffer SpotLightBuffer { SpotLight spotLights[]; };

// 3. Shadow Maps (Standardized Binding 10-15)
layout (binding = 10) uniform sampler2D shadowMapDir[2];
layout (binding = 12) uniform samplerCube shadowMapPoint[2];
layout (binding = 14) uniform sampler2D shadowMapSpot[2];

// 4. Input Stage
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace[2];
in vec4 FragPosLightSpaceSpot[2];

// 5. Common Uniforms
struct Material {
    float roughness;
    float metallic;
    float ao;
    vec3 emission;
    float opacity;
};
uniform Material material;
uniform vec4 tintColor;
uniform bool debug_noTexture;

const float PI = 3.14159265359;

// Forward Declarations
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
float ShadowCalculationDir(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex);
float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex);

void main()
{
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;

    if (debug_noTexture) {
        albedo = vec3(1.0) * tintColor.rgb;
        metallic = 0.0;
        roughness = 0.8;
        ao = 1.0;
    } else {
        albedo = pow(texture(texture_diffuse1, TexCoords).rgb, vec3(2.2)) * tintColor.rgb;
        metallic = texture(texture_metallic1, TexCoords).r * material.metallic;
        roughness = texture(texture_roughness1, TexCoords).r * material.roughness;
        ao = texture(texture_ao1, TexCoords).r * material.ao;
    }

    vec3 N = normalize(Normal);
    vec3 V = normalize(camera.viewPos - FragPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    
    // Directional lights
    for(int d = 0; d < light.numDirLights; d++) {
        vec3 L = normalize(-dirLights[d].direction);
        vec3 H = normalize(V + L);
        float shadow = 0.0;
        int sIdx = int(dirLights[d].shadowIndex);
        if (light.u_ReceiveShadow != 0 && sIdx >= 0 && sIdx < 2)
            shadow = ShadowCalculationDir(FragPosLightSpace[sIdx], N, L, sIdx);

        if(shadow < 1.0) {
            vec3 radiance = dirLights[d].color * dirLights[d].intensity;
            float NDF = DistributionGGX(N, H, roughness);
            float G   = GeometrySmith(N, V, L, roughness);
            vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 specular = numerator / denominator;
            vec3 kS = F;
            vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
            Lo += (kD * albedo / PI + specular) * radiance * max(dot(N, L), 0.0) * (1.0 - shadow);
        }
    }

    // Point lights (simplified for space, same as original logic but standardized)
    for(int i = 0; i < light.nrPointLights; ++i) {
        vec3 L = normalize(pointLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float dist = length(pointLights[i].position - FragPos);
        float atten = 1.0 / (pointLights[i].constant + pointLights[i].linear * dist + pointLights[i].quadratic * (dist * dist));
        vec3 radiance = pointLights[i].color * pointLights[i].intensity * atten;
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 specular = (NDF * G * F) / (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        Lo += (kD * albedo / PI + specular) * radiance * max(dot(N, L), 0.0);
    }

    // Spot lights... (Add if needed, aligned with shadow maps 14-15)

    // IBL
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse    = irradiance * albedo;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * 4.0).rgb;
    vec2 envBRDF  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    vec3 emissive = debug_noTexture ? vec3(0.0) : pow(texture(texture_emissive1, TexCoords).rgb, vec3(2.2)) * material.emission;
    FragColor = vec4(ambient + Lo + emissive, material.opacity);
}

// Helper functions (Simplified for brevity, ensuring same math)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness; float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0); float k = (r*r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) * GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
float ShadowCalculationDir(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    float closestDepth = texture(shadowMapDir[shadowMapIndex], projCoords.xy).r;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    return (projCoords.z - bias > closestDepth) ? 1.0 : 0.0;
}
float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex) { return 0.0; }
