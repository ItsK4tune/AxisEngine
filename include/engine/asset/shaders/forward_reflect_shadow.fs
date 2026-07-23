#version 460 core
out vec4 FragColor;

layout (binding = 0) uniform sampler2D u_AlbedoMap;
layout (binding = 1) uniform sampler2D u_NormalMap;
layout (binding = 5) uniform sampler2D u_EmissiveMap;
layout (binding = 9) uniform sampler2D u_SpecularMap;
layout (binding = 30) uniform sampler2D u_Lightmap;
uniform float u_LightmapIntensity;

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

layout (binding = 10) uniform sampler2D u_ShadowMapDir[2];
layout (binding = 12) uniform samplerCube u_ShadowMapPoint[2];
layout (binding = 14) uniform sampler2D u_ShadowMapSpot[2];

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform float u_Shininess;
uniform vec3 u_Specular;
uniform vec3 u_Emission;
uniform vec4 u_BaseColor;
uniform vec2 u_UVScale = vec2(1.0);
uniform vec2 u_UVOffset = vec2(0.0);

uniform float u_ShadowBias;
uniform int u_ShadowSoftness;

uniform float u_Reflectivity;
uniform float u_ReflectionIntensity;
uniform float u_FresnelPower;
uniform float u_FresnelBias;
uniform bool u_HasProbe;
uniform vec3 u_ProbePos;
uniform vec3 u_ProbeBoxMin;
uniform vec3 u_ProbeBoxMax;
layout (binding = 15) uniform samplerCube u_ReflectionProbe;

vec3 BoxProjection(vec3 dir, vec3 fragPos, vec3 probePos, vec3 boxMin, vec3 boxMax) {
    vec3 firstPlaneIntersect = (boxMax + probePos - fragPos) / dir;
    vec3 secondPlaneIntersect = (boxMin + probePos - fragPos) / dir;
    vec3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
    float distance = min(furthestPlane.x, min(furthestPlane.y, furthestPlane.z));
    return dir * distance + (fragPos - probePos);
}

vec3 CalcDirLight(DirLight pLight, vec3 normal, vec3 viewDir);
vec3 CalcDirLightWithShadow(DirLight pLight, vec3 normal, vec3 viewDir, int lightIndex);
vec3 CalcPointLight(PointLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir, int index);
vec3 CalcSpotLight(SpotLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir, int index);
float ShadowCalculationDir(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex);
float ShadowCalculationPoint(vec3 fragPos, vec3 u_LightPos, int index);
float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex);

void main()
{
    vec2 finalTexCoords = TexCoords * u_UVScale + u_UVOffset;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(camera.viewPos.xyz - FragPos);
    vec3 result = vec3(0.0);
    
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
    
    vec4 texColor = texture(u_AlbedoMap, finalTexCoords);
    texColor.rgb = pow(texColor.rgb, vec3(2.2));
    
    vec3 emissive = u_Emission + texture(u_EmissiveMap, finalTexCoords).rgb +
                    texture(u_Lightmap, finalTexCoords).rgb * u_LightmapIntensity;
    
    vec3 reflectionColor = vec3(0.0);
    if (u_HasProbe && u_Reflectivity > 0.0) {
        vec3 R = reflect(-viewDir, norm);
        vec3 projectedR = BoxProjection(R, FragPos, u_ProbePos, u_ProbeBoxMin, u_ProbeBoxMax);
        reflectionColor = texture(u_ReflectionProbe, projectedR).rgb;

        float F0 = u_FresnelBias;
        float cosTheta = max(dot(norm, viewDir), 0.0);
        float F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, u_FresnelPower);
        
        reflectionColor *= F * u_Reflectivity * u_ReflectionIntensity;
    }
    
    vec3 color = result + emissive + reflectionColor;
    FragColor = vec4(color, texColor.a * u_BaseColor.a);
}

vec3 CalcDirLightWithShadow(DirLight pLight, vec3 normal, vec3 viewDir, int lightIndex)
{
    vec2 finalTexCoords = TexCoords * u_UVScale + u_UVOffset;
    vec3 lightDir = normalize(-pLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);

    vec3 albedo = pow(texture(u_AlbedoMap, finalTexCoords).rgb, vec3(2.2)) * u_BaseColor.rgb;
    vec3 specularMap = texture(u_SpecularMap, finalTexCoords).rgb;

    vec3 u_Ambient  = pLight.color * pLight.u_Ambient * pLight.intensity * albedo;
    vec3 diffuse  = pLight.color * pLight.diffuse * pLight.intensity * diff * albedo;
    vec3 specularOut = pLight.color * pLight.u_Specular * pLight.intensity * spec * specularMap * u_Specular;

    vec4 fragPosLightSpace = light.lightSpaceMatricesDir[lightIndex] * vec4(FragPos, 1.0);
    float shadow = light.u_ReceiveShadow != 0 ? ShadowCalculationDir(fragPosLightSpace, normal, lightDir, lightIndex) : 0.0;
    return (u_Ambient + (1.0 - shadow) * (diffuse + specularOut));
}

vec3 CalcDirLight(DirLight pLight, vec3 normal, vec3 viewDir)
{
    vec2 finalTexCoords = TexCoords * u_UVScale + u_UVOffset;
    vec3 lightDir = normalize(-pLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);

    vec3 albedo = pow(texture(u_AlbedoMap, finalTexCoords).rgb, vec3(2.2)) * u_BaseColor.rgb;
    vec3 specularMap = texture(u_SpecularMap, finalTexCoords).rgb;

    vec3 u_Ambient  = pLight.color * pLight.u_Ambient * pLight.intensity * albedo;
    vec3 diffuse  = pLight.color * pLight.diffuse * pLight.intensity * diff * albedo;
    vec3 specularOut = pLight.color * pLight.u_Specular * pLight.intensity * spec * specularMap * u_Specular;

    return (u_Ambient + diffuse + specularOut);
}

vec3 CalcPointLight(PointLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir, int index)
{
    vec2 finalTexCoords = TexCoords * u_UVScale + u_UVOffset;
    vec3 lightDir = normalize(pLight.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
    float distance = length(pLight.position - fragPos);
    float attenuation = 1.0 / (pLight.constant + pLight.linear * distance + pLight.quadratic * (distance * distance));

    vec3 albedo = pow(texture(u_AlbedoMap, finalTexCoords).rgb, vec3(2.2)) * u_BaseColor.rgb;
    vec3 specularMap = texture(u_SpecularMap, finalTexCoords).rgb;

    vec3 u_Ambient  = pLight.color * pLight.u_Ambient * pLight.intensity * albedo * attenuation;
    vec3 diffuse  = pLight.color * pLight.diffuse * pLight.intensity * diff * albedo * attenuation;
    vec3 specularOut = pLight.color * pLight.u_Specular * pLight.intensity * spec * specularMap * u_Specular * attenuation;

    float shadow = 0.0;
    if (index >= 0 && index < 2)
        shadow = ShadowCalculationPoint(fragPos, pLight.position, index);
    return (u_Ambient + (1.0 - shadow) * (diffuse + specularOut));
}

vec3 CalcSpotLight(SpotLight pLight, vec3 normal, vec3 fragPos, vec3 viewDir, int index)
{
    vec2 finalTexCoords = TexCoords * u_UVScale + u_UVOffset;
    vec3 lightDir = normalize(pLight.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
    float distance = length(pLight.position - fragPos);
    float attenuation = 1.0 / (pLight.constant + pLight.linear * distance + pLight.quadratic * (distance * distance));
    
    float theta = dot(lightDir, normalize(-pLight.direction));
    float epsilon = pLight.cutOff - pLight.outerCutOff;
    float intensity = clamp((theta - pLight.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 albedo = pow(texture(u_AlbedoMap, finalTexCoords).rgb, vec3(2.2)) * u_BaseColor.rgb;
    vec3 specularMap = texture(u_SpecularMap, finalTexCoords).rgb;

    vec3 u_Ambient  = pLight.color * pLight.u_Ambient * pLight.intensity * albedo * attenuation * intensity;
    vec3 diffuse  = pLight.color * pLight.diffuse * pLight.intensity * diff * albedo * attenuation * intensity;
    vec3 specularOut = pLight.color * pLight.u_Specular * pLight.intensity * spec * specularMap * u_Specular * attenuation * intensity;

    float shadow = 0.0;
    if (light.u_ReceiveShadow != 0 && index >= 0 && index < 2)
    {
        vec4 fragPosLightSpaceSpot = light.lightSpaceMatricesSpot[index] * vec4(FragPos, 1.0);
        shadow = ShadowCalculationSpot(fragPosLightSpaceSpot, normal, lightDir, index);
    }
    return (u_Ambient + (1.0 - shadow) * (diffuse + specularOut));
}

float ShadowCalculationDir(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    float currentDepth = projCoords.z;
    float bias = u_ShadowBias > 0.0 ? u_ShadowBias : max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMapDir[shadowMapIndex], 0);
    if (u_ShadowSoftness == 0) {
        float closestDepth = texture(u_ShadowMapDir[shadowMapIndex], projCoords.xy).r;
        shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    } else {
        int range = u_ShadowSoftness == 2 ? 2 : 1;
        float totalSamples = pow(range * 2 + 1, 2);
        for(int x = -range; x <= range; ++x) {
            for(int y = -range; y <= range; ++y) {
                float pcfDepth = texture(u_ShadowMapDir[shadowMapIndex], projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= totalSamples;
    }
    return shadow;
}

float ShadowCalculationPoint(vec3 fragPos, vec3 u_LightPos, int index)
{
    vec3 fragToLight = fragPos - u_LightPos;
    float closestDepth = texture(u_ShadowMapPoint[index], fragToLight).r;
    closestDepth *= light.farPlanePoint;
    float currentDepth = length(fragToLight);
    return (currentDepth - 0.05 > closestDepth) ? 1.0 : 0.0;
}

float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    float closestDepth = texture(u_ShadowMapSpot[shadowMapIndex], projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMapSpot[shadowMapIndex], 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(u_ShadowMapSpot[shadowMapIndex], projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}




