#version 430 core
out vec4 FragColor;


layout (binding = 0) uniform sampler2D texture_diffuse1;
layout (binding = 1) uniform sampler2D texture_normal1;
layout (binding = 2) uniform sampler2D texture_metallic1;
layout (binding = 3) uniform sampler2D texture_roughness1;
layout (binding = 4) uniform sampler2D texture_ao1;
layout (binding = 5) uniform sampler2D texture_emissive1;
layout (binding = 6) uniform sampler2D texture_specular1;


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


in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;


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


vec3 CalcDirLight(DirLight pLight, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(camera.viewPos - FragPos);
    vec3 result = vec3(0.0);
    
    for(int i = 0; i < light.numDirLights; i++)
        result += CalcDirLight(dirLights[i], norm, viewDir);
    
    for(int i = 0; i < light.nrPointLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    
    for(int i = 0; i < light.nrSpotLights; i++)
        result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir);
    

    vec4 texColor;
    if (debug_noTexture) {
        texColor = vec4(1.0);
    } else {
        texColor = texture(texture_diffuse1, TexCoords);
        texColor.rgb = pow(texColor.rgb, vec3(2.2));
    }
    
    FragColor = vec4(result + material.emission, texColor.a * material.opacity * tintColor.a);
}

vec3 CalcDirLight(DirLight pLight, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-pLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 albedo = (debug_noTexture ? vec3(1.0) : pow(texture(texture_diffuse1, TexCoords).rgb, vec3(2.2))) * tintColor.rgb;
    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(texture_specular1, TexCoords).rgb;

    vec3 ambient  = pLight.color * 0.1 * pLight.intensity * albedo * material.ambient;
    vec3 diffuse  = pLight.color * 0.8 * pLight.intensity * diff * albedo;
    vec3 specular = pLight.color * 1.0 * pLight.intensity * spec * specularMap * material.specular;

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

    vec3 albedo = debug_noTexture ? vec3(1.0) : pow(texture(texture_diffuse1, TexCoords).rgb, vec3(2.2));
    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(texture_specular1, TexCoords).rgb;

    vec3 ambient  = pLight.color * 0.1 * pLight.intensity * albedo * material.ambient * attenuation;
    vec3 diffuse  = pLight.color * 0.8 * pLight.intensity * diff * albedo * attenuation;
    vec3 specular = pLight.color * 1.0 * pLight.intensity * spec * specularMap * material.specular * attenuation;

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

    vec3 albedo = debug_noTexture ? vec3(1.0) : pow(texture(texture_diffuse1, TexCoords).rgb, vec3(2.2));
    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(texture_specular1, TexCoords).rgb;

    vec3 ambient  = pLight.color * 0.1 * pLight.intensity * albedo * material.ambient * attenuation * intensity;
    vec3 diffuse  = pLight.color * 0.8 * pLight.intensity * diff * albedo * attenuation * intensity;
    vec3 specular = pLight.color * 1.0 * pLight.intensity * spec * specularMap * material.specular * attenuation * intensity;

    return (ambient + diffuse + specular);
}
