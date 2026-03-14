#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D gPosition;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D gAlbedoSpec;

uniform int u_DebugMode; // 0=Lit, 1=Pos, 2=Normal, 3=Albedo, 4=IDs

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

layout(std430, binding = 2) buffer DirLightBuffer {
    DirLight dirLights[];
};

layout(std430, binding = 3) buffer PointLightBuffer {
    PointLight pointLights[];
};

layout(std430, binding = 4) buffer SpotLightBuffer {
    SpotLight spotLights[];
};

layout(std140, binding = 0) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec3 viewPos;
} camera;

layout(std140, binding = 1) uniform LightData {
    mat4 lightSpaceMatricesDir[2];
    mat4 lightSpaceMatricesSpot[2];
    int numDirLights;
    int nrPointLights;
    int nrSpotLights;
    int u_ReceiveShadow;
    float farPlanePoint;
    float farPlaneSpot;
} light;

#define NR_DIR_SHADOW_MAPS 2
#define NR_SPOT_SHADOW_MAPS 2

uniform sampler2D shadowMapDir[NR_DIR_SHADOW_MAPS];
uniform sampler2D shadowMapSpot[NR_SPOT_SHADOW_MAPS];

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
float ShadowCalculationDir(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex);
float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex);

void main()
{             
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    
    // Discard background pixels where no geometry was written (FragPos will be 0)
    if (dot(FragPos, FragPos) < 0.0001) discard;

    vec3 Normal = texture(gNormal, TexCoords).rgb;
    Normal = normalize(Normal * 2.0 - 1.0); // Decode from [0, 1]
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Roughness = texture(gAlbedoSpec, TexCoords).a;
    float Metallic = 0.0;
    float AO = 1.0;

    vec3 N = normalize(Normal);
    vec3 V = normalize(camera.viewPos - FragPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, Albedo, Metallic);

    vec3 Lo = vec3(0.0);
    
    // Directional lights
    for(int d = 0; d < light.numDirLights; d++)
    {
        vec3 L = normalize(-dirLights[d].direction);
        vec3 H = normalize(V + L);
        
        float shadow = 0.0;
        int sIdx = int(dirLights[d].shadowIndex);
        if (light.u_ReceiveShadow != 0 && sIdx >= 0 && sIdx < NR_DIR_SHADOW_MAPS)
        {
            vec4 fragPosLightSpace = light.lightSpaceMatricesDir[sIdx] * vec4(FragPos, 1.0);
            shadow = ShadowCalculationDir(fragPosLightSpace, N, L, sIdx);
        }

        if(shadow < 1.0)
        {
            vec3 radiance = dirLights[d].color * dirLights[d].intensity;

            float NDF = DistributionGGX(N, H, Roughness);
            float G   = GeometrySmith(N, V, L, Roughness);
            vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 specular = numerator / denominator;

            float NdotL = max(dot(N, L), 0.0);
            Lo += (((vec3(1.0) - F) * (1.0 - Metallic) * Albedo / PI) + specular) * radiance * NdotL * (1.0 - shadow);
        }
    }

    // Point lights
    for(int i = 0; i < light.nrPointLights; ++i)
    {
        vec3 L = normalize(pointLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float distance = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * (distance * distance));
        vec3 radiance = pointLights[i].color * pointLights[i].intensity * attenuation;

        float NDF = DistributionGGX(N, H, Roughness);
        float G   = GeometrySmith(N, V, L, Roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (((vec3(1.0) - F) * (1.0 - Metallic) * Albedo / PI) + specular) * radiance * NdotL;
    }

    // Spot lights... (Skipping for brevity in this initial implementation, or add it similarly)

    vec3 color = Lo + Albedo * 0.03; // Simple ambient
    
    // Simple tone mapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}

// Lighting functions (same as pbr_lit_shadow.fs)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
float ShadowCalculationDir(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    float closestDepth = texture(shadowMapDir[shadowMapIndex], projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(0.02 * (1.0 - dot(normal, lightDir)), 0.002);
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0; // Simple shadow for now
    return shadow;
}
float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex) {
    return 0.0; // Optional spot shadow
}
