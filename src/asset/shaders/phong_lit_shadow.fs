#version 430 core
out vec4 FragColor;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
    vec3 specular;
    vec3 ambient;
    vec3 emission;
    float opacity;
};

struct DirLight {
    vec3 direction; float pad0;
    vec3 color; float intensity;
    vec3 ambient; float pad1;
    vec3 diffuse; float pad2;
    vec3 specular; float pad3;
};

struct PointLight {
    vec3 position; float pad0;
    vec3 color; float intensity;
    float constant; float linear; float quadratic; float radius;
    vec3 ambient; float pad1;
    vec3 diffuse; float pad2;
    vec3 specular; float pad3;
};

struct SpotLight {
    vec3 position; float pad0;
    vec3 direction; float pad1;
    vec3 color; float intensity;
    float cutOff; float outerCutOff; float constant; float linear;
    float quadratic; float pad2; float pad3; float pad4;
    vec3 ambient; float pad5;
    vec3 diffuse; float pad6;
    vec3 specular; float pad7;
};

layout(std430, binding = 0) buffer DirLightBuffer {
    DirLight dirLights[];
};

layout(std430, binding = 1) buffer PointLightBuffer {
    PointLight pointLights[];
};

layout(std430, binding = 2) buffer SpotLightBuffer {
    SpotLight spotLights[];
};

#define NR_POINT_LIGHTS 4
#define NR_DIR_SHADOW_MAPS 4

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace[4]; // Array for 4 directional light shadows

uniform vec3 viewPos;
uniform Material material;
uniform int numDirLights;
uniform int nrPointLights;
uniform int nrSpotLights;
uniform vec4 tintColor;
uniform bool u_ReceiveShadow;
uniform sampler2D shadowMapDir[NR_DIR_SHADOW_MAPS]; // Array of 4 shadow maps
uniform samplerCube shadowMapPoint[NR_POINT_LIGHTS];
uniform float farPlanePoint;

uniform bool debug_noTexture;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcDirLightWithShadow(DirLight light, vec3 normal, vec3 viewDir, int lightIndex);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, int index);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float ShadowCalculationDir(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex);
float ShadowCalculationPoint(vec3 fragPos, vec3 lightPos, int index);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0);
    
    // Directional lights with shadows (up to 4)
    int numShadowLights = min(numDirLights, NR_DIR_SHADOW_MAPS);
    for(int i = 0; i < numShadowLights; i++)
    {
        result += CalcDirLightWithShadow(dirLights[i], norm, viewDir, i);
    }
    
    // Remaining directional lights without shadows
    for(int i = numShadowLights; i < numDirLights; i++)
    {
        result += CalcDirLight(dirLights[i], norm, viewDir);
    }
    
    // Point lights
    for(int i = 0; i < nrPointLights && i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, i);
    
    // Spot lights
    for(int i = 0; i < nrSpotLights; i++)
        result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir);
    
    vec4 texColor;
    if (debug_noTexture) {
        texColor = vec4(1.0);
    } else {
        texColor = texture(material.texture_diffuse1, TexCoords);
    }
    
    vec3 emission = material.emission;
    FragColor = vec4(result + emission, texColor.a * material.opacity) * tintColor;
}

vec3 CalcDirLightWithShadow(DirLight light, vec3 normal, vec3 viewDir, int lightIndex)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    if (debug_noTexture) {
         ambient = light.ambient * light.intensity * vec3(1.0) * material.ambient;
         diffuse = light.diffuse * light.intensity * diff * vec3(1.0);
         specular = light.specular * light.intensity * spec * vec3(0.5) * material.specular;
    } else {
         ambient = light.ambient * light.intensity * vec3(texture(material.texture_diffuse1, TexCoords)) * material.ambient;
         diffuse = light.diffuse * light.intensity * diff * vec3(texture(material.texture_diffuse1, TexCoords));
         specular = light.specular * light.intensity * spec * vec3(texture(material.texture_specular1, TexCoords)) * material.specular;
    }

    float shadow = u_ReceiveShadow ? ShadowCalculationDir(FragPosLightSpace[lightIndex], normal, lightDir, lightIndex) : 0.0;
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

float ShadowCalculationDir(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMapDir[shadowMapIndex], projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMapDir[shadowMapIndex], 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMapDir[shadowMapIndex], projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    if (debug_noTexture) {
         ambient = light.ambient * light.intensity * vec3(1.0) * material.ambient;
         diffuse = light.diffuse * light.intensity * diff * vec3(1.0);
         specular = light.specular * light.intensity * spec * vec3(0.5) * material.specular;
    } else {
         ambient = light.ambient * light.intensity * vec3(texture(material.texture_diffuse1, TexCoords)) * material.ambient;
         diffuse = light.diffuse * light.intensity * diff * vec3(texture(material.texture_diffuse1, TexCoords));
         specular = light.specular * light.intensity * spec * vec3(texture(material.texture_specular1, TexCoords)) * material.specular;
    }

    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, int index)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    if (debug_noTexture) {
         ambient = light.ambient * light.intensity * vec3(1.0) * material.ambient;
         diffuse = light.diffuse * light.intensity * diff * vec3(1.0);
         specular = light.specular * light.intensity * spec * vec3(0.5) * material.specular;
    } else {
         ambient = light.ambient * light.intensity * vec3(texture(material.texture_diffuse1, TexCoords)) * material.ambient;
         diffuse = light.diffuse * light.intensity * diff * vec3(texture(material.texture_diffuse1, TexCoords));
         specular = light.specular * light.intensity * spec * vec3(texture(material.texture_specular1, TexCoords)) * material.specular;
    }

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    float shadow = ShadowCalculationPoint(fragPos, light.position, index);
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

float ShadowCalculationPoint(vec3 fragPos, vec3 lightPos, int index)
{
    vec3 fragToLight = fragPos - lightPos;
    float closestDepth = texture(shadowMapPoint[index], fragToLight).r;
    closestDepth *= farPlanePoint;
    float currentDepth = length(fragToLight);
    float bias = 0.05;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    return shadow;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    if (debug_noTexture) {
         ambient = light.ambient * light.intensity * vec3(1.0) * material.ambient;
         diffuse = light.diffuse * light.intensity * diff * vec3(1.0);
         specular = light.specular * light.intensity * spec * vec3(0.5) * material.specular;
    } else {
         ambient = light.ambient * light.intensity * vec3(texture(material.texture_diffuse1, TexCoords)) * material.ambient;
         diffuse = light.diffuse * light.intensity * diff * vec3(texture(material.texture_diffuse1, TexCoords));
         specular = light.specular * light.intensity * spec * vec3(texture(material.texture_specular1, TexCoords)) * material.specular;
    }

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}
