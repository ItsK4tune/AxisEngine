#version 430 core
out vec4 FragColor;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
    vec3 specular;
};

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

layout(std430, binding = 0) buffer DirLightBuffer {
    DirLight dirLights[];
};

layout(std430, binding = 1) buffer PointLightBuffer {
    PointLight pointLights[];
};

layout(std430, binding = 2) buffer SpotLightBuffer {
    SpotLight spotLights[];
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
// uniform DirLight dirLight;
// uniform DirLight dirLights[NR_DIR_LIGHTS];
// uniform PointLight pointLights[NR_POINT_LIGHTS];
// uniform SpotLight spotLights[NR_SPOT_LIGHTS];
uniform Material material;
uniform int numDirLights;
uniform int nrPointLights;
uniform int nrSpotLights;
uniform vec4 tintColor;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

uniform bool debug_noTexture;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0);
    
    // Directional
    for(int i = 0; i < numDirLights; i++)
        result += CalcDirLight(dirLights[i], norm, viewDir);

    // Point
    for(int i = 0; i < nrPointLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

    // Spot
    for(int i = 0; i < nrSpotLights; i++)
        result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir);

    FragColor = vec4(result, 1.0) * tintColor;
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
         ambient = light.ambient * light.intensity * vec3(1.0);
         diffuse = light.diffuse * light.intensity * diff * vec3(1.0);
         specular = light.specular * light.intensity * spec * vec3(0.5) * material.specular;
    } else {
         ambient = light.ambient * light.intensity * vec3(texture(material.texture_diffuse1, TexCoords));
         diffuse = light.diffuse * light.intensity * diff * vec3(texture(material.texture_diffuse1, TexCoords));
         specular = light.specular * light.intensity * spec * vec3(texture(material.texture_specular1, TexCoords)) * material.specular;
    }
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
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
         ambient = light.ambient * light.intensity * vec3(1.0);
         diffuse = light.diffuse * light.intensity * diff * vec3(1.0);
         specular = light.specular * light.intensity * spec * vec3(0.5) * material.specular;
    } else {
         ambient = light.ambient * light.intensity * vec3(texture(material.texture_diffuse1, TexCoords));
         diffuse = light.diffuse * light.intensity * diff * vec3(texture(material.texture_diffuse1, TexCoords));
         specular = light.specular * light.intensity * spec * vec3(texture(material.texture_specular1, TexCoords)) * material.specular;
    }
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
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
         ambient = light.ambient * light.intensity * vec3(1.0);
         diffuse = light.diffuse * light.intensity * diff * vec3(1.0);
         specular = light.specular * light.intensity * spec * vec3(0.5) * material.specular;
    } else {
         ambient = light.ambient * light.intensity * vec3(texture(material.texture_diffuse1, TexCoords));
         diffuse = light.diffuse * light.intensity * diff * vec3(texture(material.texture_diffuse1, TexCoords));
         specular = light.specular * light.intensity * spec * vec3(texture(material.texture_specular1, TexCoords)) * material.specular;
    }
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}
