#version 430 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D u_DecalAlbedo;
uniform bool u_HasDecalTexture;
uniform float u_Opacity;
uniform vec4 u_TintColor;
uniform int u_LightingMode; // 0=NONE, 1=LIGHT, 2=LIGHT+SHADOW

uniform float u_Roughness;
uniform float u_Metallic;
uniform float u_Reflectivity;

layout(binding = 10) uniform sampler2DArray u_ShadowMapDir;
layout(binding = 11) uniform samplerCubeArray u_ShadowMapPoint;
layout(binding = 12) uniform sampler2DArray u_ShadowMapSpot;

uniform float u_ShadowBias = 0.005;
uniform int u_ShadowSoftness = 1;

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

float ShadowCalculationDir(vec4 fragPosLightSpace, int lightIdx);
float ShadowCalculationPoint(vec3 fragPos, vec3 u_LightPos, int lightIdx);
float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int lightIdx);

void main()
{
    vec4 texColor = u_HasDecalTexture ? texture(u_DecalAlbedo, TexCoords) : vec4(1.0);
    if (texColor.a < 0.1) discard;

    vec3 Albedo = pow(texColor.rgb, vec3(2.2)) * u_TintColor.rgb;
    float finalAlpha = texColor.a * u_Opacity * u_TintColor.a;

    // NONE (Unlit) mode
    if (u_LightingMode == 0) {
        FragColor = vec4(Albedo, finalAlpha);
        return;
    }

    vec3 N = normalize(Normal);
    vec3 V = normalize(camera.viewPos.xyz - FragPos);

    vec3 Lo = vec3(0.0);
    
    // Direct Lighting (Dir Lights)
    for(int i = 0; i < light.numDirLights; ++i) {
        vec3 L = normalize(-dirLights[i].direction);
        vec3 H = normalize(V + L);
        float diff = max(dot(N, L), 0.0);
        
        float specPower = mix(128.0, 2.0, u_Roughness);
        float spec = pow(max(dot(N, H), 0.0), specPower) * (1.0 - u_Roughness);
        
        float shadow = 0.0;
        int sIdx = int(dirLights[i].shadowIndex);
        if (u_LightingMode == 2 && sIdx != -1) {
            shadow = ShadowCalculationDir(light.lightSpaceMatricesDir[sIdx] * vec4(FragPos, 1.0), sIdx);
        }
        
        vec3 radiance = dirLights[i].color * dirLights[i].intensity;
        vec3 diffuse  = dirLights[i].diffuse * radiance * diff * Albedo;
        vec3 u_Specular = dirLights[i].u_Specular * radiance * spec * 0.5 * (u_Reflectivity > 0.0 ? 1.0 : 0.0); 
        
        Lo += (1.0 - shadow) * (diffuse + u_Specular);
    }

    // Direct Lighting (Point Lights)
    for(int i = 0; i < light.nrPointLights; ++i) {
        vec3 L = normalize(pointLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float dist = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * dist + pointLights[i].quadratic * (dist * dist));
        float diff = max(dot(N, L), 0.0);
        
        float specPower = mix(128.0, 2.0, u_Roughness);
        float spec = pow(max(dot(N, H), 0.0), specPower) * (1.0 - u_Roughness);
        
        float shadow = 0.0;
        int sIdx = int(pointLights[i].shadowIndex);
        if (u_LightingMode == 2 && sIdx != -1) {
            shadow = ShadowCalculationPoint(FragPos, pointLights[i].position, sIdx);
        }
        
        vec3 radiance = pointLights[i].color * pointLights[i].intensity * attenuation;
        vec3 diffuse  = pointLights[i].diffuse * radiance * diff * Albedo;
        vec3 u_Specular = pointLights[i].u_Specular * radiance * spec * 0.5 * (u_Reflectivity > 0.0 ? 1.0 : 0.0);
        
        Lo += (1.0 - shadow) * (diffuse + u_Specular);
    }

    // Direct Lighting (Spot Lights)
    for(int i = 0; i < light.nrSpotLights; ++i) {
        vec3 L = normalize(spotLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float diff = max(dot(N, L), 0.0);
        float theta = dot(L, normalize(-spotLights[i].direction));
        float epsilon = spotLights[i].cutOff - spotLights[i].outerCutOff;
        float spotIntensity = clamp((theta - spotLights[i].outerCutOff) / epsilon, 0.0, 1.0);
        float dist = length(spotLights[i].position - FragPos);
        float attenuation = 1.0 / (spotLights[i].constant + spotLights[i].linear * dist + spotLights[i].quadratic * (dist * dist));
        
        float specPower = mix(128.0, 2.0, u_Roughness);
        float spec = pow(max(dot(N, H), 0.0), specPower) * (1.0 - u_Roughness);
        
        float shadow = 0.0;
        int sIdx = int(spotLights[i].shadowIndex);
        if (u_LightingMode == 2 && sIdx != -1) {
            shadow = ShadowCalculationSpot(light.lightSpaceMatricesSpot[sIdx] * vec4(FragPos, 1.0), N, L, sIdx);
        }
        
        vec3 radiance = spotLights[i].color * spotLights[i].intensity * attenuation * spotIntensity;
        vec3 diffuse  = spotLights[i].diffuse * radiance * diff * Albedo;
        vec3 u_Specular = spotLights[i].u_Specular * radiance * spec * 0.5 * (u_Reflectivity > 0.0 ? 1.0 : 0.0);
        
        Lo += (1.0 - shadow) * (diffuse + u_Specular);
    }

    // Baseline u_Ambient
    vec3 u_Ambient = Albedo * 0.15; 
    
    FragColor = vec4(u_Ambient + Lo, finalAlpha);
}

float ShadowCalculationDir(vec4 fragPosLightSpace, int lightIdx) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    float currentDepth = projCoords.z;
    float bias = u_ShadowBias;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMapDir, 0).xy;
    for(int x = -u_ShadowSoftness; x <= u_ShadowSoftness; ++x) {
        for(int y = -u_ShadowSoftness; y <= u_ShadowSoftness; ++y) {
            float pcfDepth = texture(u_ShadowMapDir, vec3(projCoords.xy + vec2(x, y) * texelSize, lightIdx)).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= float((u_ShadowSoftness * 2 + 1) * (u_ShadowSoftness * 2 + 1));
    return shadow;
}

float ShadowCalculationPoint(vec3 fragPos, vec3 u_LightPos, int lightIdx) {
    vec3 fragToLight = fragPos - u_LightPos;
    float currentDepth = length(fragToLight);
    float shadow = 0.0;
    float bias = u_ShadowBias * 10.0;
    int samples = 20;
    vec3 sampleOffsetDirections[20] = vec3[] (
       vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
       vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
       vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
       vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
       vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );
    float diskRadius = (1.0 + (currentDepth / light.farPlanePoint)) / 25.0;
    for(int i = 0; i < samples; ++i) {
        float closestDepth = texture(u_ShadowMapPoint, vec4(fragToLight + sampleOffsetDirections[i] * diskRadius, lightIdx)).r;
        closestDepth *= light.farPlanePoint;
        if(currentDepth - bias > closestDepth) shadow += 1.0;
    }
    shadow /= float(samples);
    return shadow;
}

float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int lightIdx) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    if (projCoords.z > 1.0) return 0.0;
    float currentDepth = projCoords.z;
    float ndotl = max(dot(normalize(normal), normalize(lightDir)), 0.0);
    float bias = max(u_ShadowBias * 0.25 * (1.0 - ndotl), 0.00005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMapSpot, 0).xy;
    for(int x = -u_ShadowSoftness; x <= u_ShadowSoftness; ++x) {
        for(int y = -u_ShadowSoftness; y <= u_ShadowSoftness; ++y) {
            float pcfDepth = texture(u_ShadowMapSpot, vec3(projCoords.xy + vec2(x, y) * texelSize, lightIdx)).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= float((u_ShadowSoftness * 2 + 1) * (u_ShadowSoftness * 2 + 1));
    return shadow;
}
