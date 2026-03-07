#version 430 core
out vec4 FragColor;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
    vec3 specular;
    float opacity;
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

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform Material material;
uniform vec4 tintColor;

vec3 CalcDirLight(DirLight pLight, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir);

uniform bool debug_noTexture;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(camera.viewPos - FragPos);

    vec3 result = vec3(0.0);
    
    // Directional
    for(int i = 0; i < light.numDirLights; i++)
        result += CalcDirLight(dirLights[i], norm, viewDir);

    // Point
    for(int i = 0; i < light.nrPointLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

    // Spot
    for(int i = 0; i < light.nrSpotLights; i++)
        result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir);

    FragColor = vec4(result, material.opacity) * tintColor;
}

vec3 CalcDirLight(DirLight pLight, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-pLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    if (debug_noTexture) {
         ambient = pLight.color * 0.1 * pLight.intensity * vec3(1.0);
         diffuse = pLight.color * 0.8 * pLight.intensity * diff * vec3(1.0);
         specular = pLight.color * 1.0 * pLight.intensity * spec * vec3(0.5) * material.specular;
    } else {
         ambient = pLight.color * 0.1 * pLight.intensity * vec3(texture(material.texture_diffuse1, TexCoords));
         diffuse = pLight.color * 0.8 * pLight.intensity * diff * vec3(texture(material.texture_diffuse1, TexCoords));
         specular = pLight.color * 1.0 * pLight.intensity * spec * vec3(texture(material.texture_specular1, TexCoords)) * material.specular;
    }
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(pLight.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float distance = length(pLight.position - fragPos);
    float attenuation = 1.0 / (pLight.constant + pLight.linear * distance + pLight.quadratic * (distance * distance));
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    if (debug_noTexture) {
         ambient = pLight.color * 0.1 * pLight.intensity * vec3(1.0);
         diffuse = pLight.color * 0.8 * pLight.intensity * diff * vec3(1.0);
         specular = pLight.color * 1.0 * pLight.intensity * spec * vec3(0.5) * material.specular;
    } else {
         ambient = pLight.color * 0.1 * pLight.intensity * vec3(texture(material.texture_diffuse1, TexCoords));
         diffuse = pLight.color * 0.8 * pLight.intensity * diff * vec3(texture(material.texture_diffuse1, TexCoords));
         specular = pLight.color * 1.0 * pLight.intensity * spec * vec3(texture(material.texture_specular1, TexCoords)) * material.specular;
    }
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir)
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
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    if (debug_noTexture) {
         ambient = pLight.color * 0.1 * pLight.intensity * vec3(1.0);
         diffuse = pLight.color * 0.8 * pLight.intensity * diff * vec3(1.0);
         specular = pLight.color * 1.0 * pLight.intensity * spec * vec3(0.5) * material.specular;
    } else {
         ambient = pLight.color * 0.1 * pLight.intensity * vec3(texture(material.texture_diffuse1, TexCoords));
         diffuse = pLight.color * 0.8 * pLight.intensity * diff * vec3(texture(material.texture_diffuse1, TexCoords));
         specular = pLight.color * 1.0 * pLight.intensity * spec * vec3(texture(material.texture_specular1, TexCoords)) * material.specular;
    }
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}
