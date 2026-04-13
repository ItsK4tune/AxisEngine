#version 430 core
out vec4 FragColor;

layout (binding = 0) uniform sampler2D u_AlbedoMap;
layout (binding = 1) uniform sampler2D u_NormalMap;
layout (binding = 5) uniform sampler2D u_EmissiveMap;
layout (binding = 6) uniform sampler2D u_SpecularMap;

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
    float quadratic; float pad2; float pad3; float pad4;
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

uniform bool u_ProbeUnlit;

uniform float u_Shininess;
uniform vec3 u_Specular;
uniform vec3 u_Emission;
uniform vec4 u_BaseColor;
uniform bool debug_noTexture;

vec3 CalcDirLight(DirLight pLight, vec3 normal, vec3 V, vec3 albedo);
vec3 CalcPointLight(PointLight pLight, vec3 normal, vec3 fragPos, vec3 V, vec3 albedo);
vec3 CalcSpotLight(SpotLight pLight, vec3 normal, vec3 fragPos, vec3 V, vec3 albedo);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 V = normalize(camera.viewPos.xyz - FragPos);
    vec3 result = vec3(0.0);
    
    vec4 texColor;
    if (debug_noTexture) {
        texColor = vec4(1.0);
    } else {
        texColor = texture(u_AlbedoMap, TexCoords);
        texColor.rgb = pow(texColor.rgb, vec3(2.2));
    }
    vec3 albedo = texColor.rgb * u_BaseColor.rgb;
    
    for(int i = 0; i < light.numDirLights; i++)
        result += CalcDirLight(dirLights[i], norm, V, albedo);
    
    for(int i = 0; i < light.nrPointLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, V, albedo);
    
    for(int i = 0; i < light.nrSpotLights; i++)
        result += CalcSpotLight(spotLights[i], norm, FragPos, V, albedo);
    
    vec3 emissive = u_Emission + texture(u_EmissiveMap, TexCoords).rgb;
    
    vec3 color = result + emissive;

    if (u_ProbeUnlit) {
        color = albedo;
    }

    FragColor = vec4(color, texColor.a * u_BaseColor.a);
}

vec3 CalcDirLight(DirLight pLight, vec3 normal, vec3 V, vec3 albedo)
{
    vec3 lightDir = normalize(-pLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(V, reflectDir), 0.0), u_Shininess);

    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(u_SpecularMap, TexCoords).rgb;

    vec3 u_Ambient  = pLight.u_Ambient * pLight.color * pLight.intensity * albedo;
    vec3 diffuse  = pLight.diffuse * pLight.color * pLight.intensity * diff * albedo;
    vec3 u_Specular = pLight.u_Specular * pLight.color * pLight.intensity * spec * specularMap * u_Specular;

    return (u_Ambient + diffuse + u_Specular);
}

vec3 CalcPointLight(PointLight pLight, vec3 normal, vec3 fragPos, vec3 V, vec3 albedo)
{
    vec3 lightDir = normalize(pLight.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(V, reflectDir), 0.0), u_Shininess);
    float distance = length(pLight.position - fragPos);
    float attenuation = 1.0 / (pLight.constant + pLight.linear * distance + pLight.quadratic * (distance * distance));

    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(u_SpecularMap, TexCoords).rgb;

    vec3 u_Ambient  = pLight.u_Ambient * pLight.color * pLight.intensity * albedo * attenuation;
    vec3 diffuse  = pLight.diffuse * pLight.color * pLight.intensity * diff * albedo * attenuation;
    vec3 u_Specular = pLight.u_Specular * pLight.color * pLight.intensity * spec * specularMap * u_Specular * attenuation;

    return (u_Ambient + diffuse + u_Specular);
}

vec3 CalcSpotLight(SpotLight pLight, vec3 normal, vec3 fragPos, vec3 V, vec3 albedo)
{
    vec3 lightDir = normalize(pLight.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(V, reflectDir), 0.0), u_Shininess);
    float distance = length(pLight.position - fragPos);
    float attenuation = 1.0 / (pLight.constant + pLight.linear * distance + pLight.quadratic * (distance * distance));
    
    float theta = dot(lightDir, normalize(-pLight.direction));
    float epsilon = pLight.cutOff - pLight.outerCutOff;
    float intensity = clamp((theta - pLight.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(u_SpecularMap, TexCoords).rgb;

    vec3 u_Ambient  = pLight.u_Ambient * pLight.color * pLight.intensity * albedo * attenuation * intensity;
    vec3 diffuse  = pLight.diffuse * pLight.color * pLight.intensity * diff * albedo * attenuation * intensity;
    vec3 u_Specular = pLight.u_Specular * pLight.color * pLight.intensity * spec * specularMap * u_Specular * attenuation * intensity;

    return (u_Ambient + diffuse + u_Specular);
}



