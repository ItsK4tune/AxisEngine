#version 430 core
out vec4 FragColor;

layout (binding = 0) uniform sampler2D u_AlbedoMap;
layout (binding = 1) uniform sampler2D u_NormalMap;
layout (binding = 5) uniform sampler2D u_EmissiveMap;
layout (binding = 6) uniform sampler2D u_SpecularMap;

layout(std140, binding = 20) uniform CameraData {
    mat4 projection;
    mat4 view;
    vec4 viewPos;
    mat4 invProjection;
    mat4 invView;
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

uniform float u_Shininess;
uniform vec3 u_Specular;
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
layout (binding = 15) uniform samplerCube reflectionProbe;
layout (binding = 19) uniform sampler2D u_PlanarReflections[4];
uniform int u_PlanarCount;
uniform vec3 u_PlanarNormals[4];
uniform vec2 u_ScreenSize;

vec3 BoxProjection(vec3 dir, vec3 fragPos, vec3 probePos, vec3 boxMin, vec3 boxMax) {
    vec3 firstPlaneIntersect = (boxMax + probePos - fragPos) / dir;
    vec3 secondPlaneIntersect = (boxMin + probePos - fragPos) / dir;
    vec3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
    float distance = min(furthestPlane.x, min(furthestPlane.y, furthestPlane.z));
    return dir * distance + (fragPos - probePos);
}

vec3 CalcDirLight(DirLight pLight, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(camera.viewPos.xyz - FragPos);
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
        texColor = texture(u_AlbedoMap, TexCoords);
        texColor.rgb = pow(texColor.rgb, vec3(2.2));
    }
    
    vec3 emissive = u_Emission + texture(u_EmissiveMap, TexCoords).rgb;
    
    vec3 reflectionColor = vec3(0.0);
    float finalReflectivity = u_Reflectivity;

    // 1. Planar Reflections (Higher priority)
    float bestPlanarMask = 0.0;
    if (u_PlanarCount > 0 && u_Reflectivity > 0.0) {
        vec2 uv = gl_FragCoord.xy / u_ScreenSize;

        for (int i = 0; i < u_PlanarCount; ++i) {
            float mask = clamp((dot(norm, u_PlanarNormals[i]) - 0.98) * 50.0, 0.0, 1.0);
            if (mask > 0.0) {
                vec3 sampleColor = texture(u_PlanarReflections[i], uv).rgb;
                reflectionColor += sampleColor * mask;
                bestPlanarMask = max(bestPlanarMask, mask);
            }
        }
    }

    // 2. Fallback to Probes
    if (u_HasProbe && u_Reflectivity > 0.0 && bestPlanarMask < 1.0) {
        vec3 R = reflect(-viewDir, norm);
        vec3 projectedR = BoxProjection(R, FragPos, u_ProbePos, u_ProbeBoxMin, u_ProbeBoxMax);
        vec3 probeColor = texture(reflectionProbe, projectedR).rgb;
        reflectionColor = mix(probeColor, reflectionColor, bestPlanarMask);
    }

    if (u_Reflectivity > 0.0) {
        float F0 = u_FresnelBias;
        float cosTheta = max(dot(norm, viewDir), 0.0);
        float F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, u_FresnelPower);
        reflectionColor *= F * u_Reflectivity * u_ReflectionIntensity;
    }
    
    vec3 color = result + emissive + reflectionColor;
    FragColor = vec4(color, texColor.a * u_BaseColor.a);
}

vec3 CalcDirLight(DirLight pLight, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-pLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);

    vec3 albedo = (debug_noTexture ? vec3(1.0) : pow(texture(u_AlbedoMap, TexCoords).rgb, vec3(2.2))) * u_BaseColor.rgb;
    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(u_SpecularMap, TexCoords).rgb;

    vec3 ambient  = pLight.ambient * pLight.color * pLight.intensity * albedo;
    vec3 diffuse  = pLight.diffuse * pLight.color * pLight.intensity * diff * albedo;
    vec3 specular = pLight.specular * pLight.color * pLight.intensity * spec * specularMap * u_Specular;

    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(pLight.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
    float distance = length(pLight.position - fragPos);
    float attenuation = 1.0 / (pLight.constant + pLight.linear * distance + pLight.quadratic * (distance * distance));

    vec3 albedo = debug_noTexture ? vec3(1.0) : pow(texture(u_AlbedoMap, TexCoords).rgb, vec3(2.2)) * u_BaseColor.rgb;
    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(u_SpecularMap, TexCoords).rgb;

    vec3 ambient  = pLight.ambient * pLight.color * pLight.intensity * albedo * attenuation;
    vec3 diffuse  = pLight.diffuse * pLight.color * pLight.intensity * diff * albedo * attenuation;
    vec3 specular = pLight.specular * pLight.color * pLight.intensity * spec * specularMap * u_Specular * attenuation;

    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(pLight.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
    float distance = length(pLight.position - fragPos);
    float attenuation = 1.0 / (pLight.constant + pLight.linear * distance + pLight.quadratic * (distance * distance));
    
    float theta = dot(lightDir, normalize(-pLight.direction));
    float epsilon = pLight.cutOff - pLight.outerCutOff;
    float intensity = clamp((theta - pLight.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 albedo = debug_noTexture ? vec3(1.0) : pow(texture(u_AlbedoMap, TexCoords).rgb, vec3(2.2)) * u_BaseColor.rgb;
    vec3 specularMap = debug_noTexture ? vec3(0.5) : texture(u_SpecularMap, TexCoords).rgb;

    vec3 ambient  = pLight.ambient * pLight.color * pLight.intensity * albedo * attenuation * intensity;
    vec3 diffuse  = pLight.diffuse * pLight.color * pLight.intensity * diff * albedo * attenuation * intensity;
    vec3 specular = pLight.specular * pLight.color * pLight.intensity * spec * specularMap * u_Specular * attenuation * intensity;

    return (ambient + diffuse + specular);
}




