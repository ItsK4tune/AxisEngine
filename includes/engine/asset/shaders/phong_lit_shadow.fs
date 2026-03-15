#version 430 core
out vec4 FragColor;

// 1. Root Level Samplers (Standardized Binding 0-15)
layout (binding = 0) uniform sampler2D texture_diffuse1;
layout (binding = 1) uniform sampler2D texture_normal1;
layout (binding = 2) uniform sampler2D texture_metallic1;
layout (binding = 3) uniform sampler2D texture_roughness1;
layout (binding = 4) uniform sampler2D texture_ao1;
layout (binding = 5) uniform sampler2D texture_emissive1;
layout (binding = 6) uniform sampler2D texture_specular1;

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
    float shininess;
    vec3 specular;
    vec3 ambient;
    vec3 emission;
    float opacity;
};
uniform Material material;
uniform vec4 tintColor;
uniform bool debug_noTexture;

// Forward Declarations
vec3 CalcDirLight(DirLight pLight, vec3 normal, vec3 viewDir);
vec3 CalcDirLightWithShadow(DirLight pLight, vec3 normal, vec3 viewDir, int lightIndex);
vec3 CalcPointLight(PointLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir, int index);
vec3 CalcSpotLight(SpotLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir, int index);
float ShadowCalculationDir(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex);
float ShadowCalculationPoint(vec3 fragPos, vec3 lightPos, int index);
float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(camera.viewPos - FragPos);
    vec3 result = vec3(0.0);
    
    // 1. Lighting Calculation
    for(int i = 0; i < light.numDirLights; i++) {
        int sIdx = int(dirLights[i].shadowIndex);
        if (sIdx >= 0 && sIdx < 2)
             result += CalcDirLightWithShadow(dirLights[i], norm, viewDir, sIdx);
        else
             result += CalcDirLight(dirLights[i], norm, viewDir);
    }
    
    for(int i = 0; i < light.nrPointLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, int(pointLights[i].shadowIndex));
    
    for(int i = 0; i < light.nrSpotLights; i++)
        result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir, int(spotLights[i].shadowIndex));
    
    // 2. Final Color Assembly
    vec4 texColor;
    if (debug_noTexture) {
        texColor = vec4(1.0);
    } else {
        texColor = texture(texture_diffuse1, TexCoords);
        texColor.rgb = pow(texColor.rgb, vec3(2.2));
    }
    
    FragColor = vec4(result + material.emission, texColor.a * material.opacity) * tintColor;
}

vec3 CalcDirLightWithShadow(DirLight pLight, vec3 normal, vec3 viewDir, int lightIndex)
{
    vec3 lightDir = normalize(-pLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 albedo = debug_noTexture ? vec3(1.0) : pow(texture(texture_diffuse1, TexCoords).rgb, vec3(2.2));
    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(texture_specular1, TexCoords).rgb;

    vec3 ambient  = pLight.color * 0.1 * pLight.intensity * albedo * material.ambient;
    vec3 diffuse  = pLight.color * 0.8 * pLight.intensity * diff * albedo;
    vec3 specular = pLight.color * 1.0 * pLight.intensity * spec * specularMap * material.specular;

    float shadow = light.u_ReceiveShadow != 0 ? ShadowCalculationDir(FragPosLightSpace[lightIndex], normal, lightDir, lightIndex) : 0.0;
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

vec3 CalcDirLight(DirLight pLight, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-pLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 albedo = debug_noTexture ? vec3(1.0) : pow(texture(texture_diffuse1, TexCoords).rgb, vec3(2.2));
    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(texture_specular1, TexCoords).rgb;

    vec3 ambient  = pLight.color * 0.1 * pLight.intensity * albedo * material.ambient;
    vec3 diffuse  = pLight.color * 0.8 * pLight.intensity * diff * albedo;
    vec3 specular = pLight.color * 1.0 * pLight.intensity * spec * specularMap * material.specular;

    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir, int index)
{
    vec3 lightDir = normalize(pLight.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float distance = length(pLight.position - fragPos);
    float attenuation = 1.0 / (pLight.constant + pLight.linear * distance + pLight.quadratic * (distance * distance));

    vec3 albedo = debug_noTexture ? vec3(1.0) : pow(texture(texture_diffuse1, TexCoords).rgb, vec3(2.2));
    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(texture_specular1, TexCoords).rgb;

    vec3 ambient  = pLight.color * 0.1 * pLight.intensity * albedo * material.ambient * attenuation;
    vec3 diffuse  = pLight.color * 0.8 * pLight.intensity * diff * albedo * attenuation;
    vec3 specular = pLight.color * 1.0 * pLight.intensity * spec * specularMap * material.specular * attenuation;

    float shadow = 0.0;
    if (index >= 0 && index < 2)
        shadow = ShadowCalculationPoint(fragPos, pLight.position, index);
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

vec3 CalcSpotLight(SpotLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir, int index)
{
    vec3 lightDir = normalize(pLight.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float distance = length(pLight.position - fragPos);
    float attenuation = 1.0 / (pLight.constant + pLight.linear * distance + pLight.quadratic * (distance * distance));
    
    float theta = dot(lightDir, normalize(-pLight.direction));
    float epsilon = pLight.cutOff - pLight.outerCutOff;
    float intensity = clamp((theta - pLight.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 albedo = debug_noTexture ? vec3(1.0) : pow(texture(texture_diffuse1, TexCoords).rgb, vec3(2.2));
    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(texture_specular1, TexCoords).rgb;

    vec3 ambient  = pLight.color * 0.1 * pLight.intensity * albedo * material.ambient * attenuation * intensity;
    vec3 diffuse  = pLight.color * 0.8 * pLight.intensity * diff * albedo * attenuation * intensity;
    vec3 specular = pLight.color * 1.0 * pLight.intensity * spec * specularMap * material.specular * attenuation * intensity;

    float shadow = 0.0;
    if (light.u_ReceiveShadow != 0 && index >= 0 && index < 2)
    {
        shadow = ShadowCalculationSpot(FragPosLightSpaceSpot[index], normal, lightDir, index);
    }
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

float ShadowCalculationDir(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    float closestDepth = texture(shadowMapDir[shadowMapIndex], projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMapDir[shadowMapIndex], 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMapDir[shadowMapIndex], projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

float ShadowCalculationPoint(vec3 fragPos, vec3 lightPos, int index)
{
    vec3 fragToLight = fragPos - lightPos;
    float closestDepth = texture(shadowMapPoint[index], fragToLight).r;
    closestDepth *= light.farPlanePoint;
    float currentDepth = length(fragToLight);
    return (currentDepth - 0.05 > closestDepth) ? 1.0 : 0.0;
}

float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    float closestDepth = texture(shadowMapSpot[shadowMapIndex], projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMapSpot[shadowMapIndex], 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMapSpot[shadowMapIndex], projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}
