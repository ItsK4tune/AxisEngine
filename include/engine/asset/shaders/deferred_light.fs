#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D gPosition;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D gAlbedoSpec;
layout (binding = 3) uniform usampler2D gID;
layout (binding = 4) uniform sampler2D gEmissive;
layout (binding = 5) uniform sampler2D gPBRParams;
layout (binding = 6) uniform sampler2D gDepth;

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

layout(binding = 10) uniform sampler2DArray shadowMapDir;
layout(binding = 11) uniform samplerCubeArray shadowMapPoint;
layout(binding = 12) uniform sampler2DArray shadowMapSpot;

uniform int u_DebugMode; 
uniform float u_ShadowBias = 0.005;
uniform int u_ShadowSoftness = 1;

layout(binding = 15) uniform samplerCube reflectionProbes[4];
uniform int u_ProbeCount = 0;
struct ProbeData {
    vec3 pos;
    vec3 boxMin;
    vec3 boxMax;
};
uniform ProbeData u_Probes[4];

vec3 BoxProjection(vec3 dir, vec3 pos, vec3 probePos, vec3 boxMin, vec3 boxMax) {
    vec3 firstPlaneIntersect = (boxMax - pos) / dir;
    vec3 secondPlaneIntersect = (boxMin - pos) / dir;
    vec3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
    float dist = min(furthestPlane.x, min(furthestPlane.y, furthestPlane.z));
    return dir * dist + (pos - probePos);
}

uniform vec3 u_SH[9];
uniform bool u_HasLightProbe = false;
uniform float u_LightProbeIntensity = 1.0;

vec3 EvaluateSH(vec3 n) {
    const float C1 = 0.429043;
    const float C2 = 0.511664;
    const float C3 = 0.743125;
    const float C4 = 0.886227;
    const float C5 = 0.247708;

    return max(vec3(0.0), (
        C4 * u_SH[0] -
        C2 * n.y * u_SH[1] +
        C2 * n.z * u_SH[2] -
        C2 * n.x * u_SH[3] +
        C1 * n.y * n.x * u_SH[4] -
        C1 * n.y * n.z * u_SH[5] +
        C3 * (n.z * n.z - 1.0/3.0) * u_SH[6] -
        C1 * n.z * n.x * u_SH[7] +
        C5 * (n.x * n.x - n.y * n.y) * u_SH[8]
    ));
}

vec3 WorldPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(TexCoords * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = camera.invStableProjection * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = camera.invView * viewSpacePosition;
    return worldSpacePosition.xyz;
}

float ShadowCalculationDir(vec4 fragPosLightSpace, int lightIdx);
float ShadowCalculationPoint(vec3 fragPos, vec3 lightPos, int lightIdx);
float ShadowCalculationSpot(vec4 fragPosLightSpace, int lightIdx);

void main()
{             
    float depth = texture(gDepth, TexCoords).r;
    if (depth >= 1.0) discard;
    vec3 FragPos = WorldPosFromDepth(depth);

    vec3 Normal = texture(gNormal, TexCoords).rgb;
    Normal = normalize(Normal);
    
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    uint EntityID = texture(gID, TexCoords).r;

    if (u_DebugMode == 1) { FragColor = vec4(FragPos, 1.0); return; }
    if (u_DebugMode == 2) { FragColor = vec4(Normal * 0.5 + 0.5, 1.0); return; }
    if (u_DebugMode == 3) { FragColor = vec4(Albedo, 1.0); return; }
    if (u_DebugMode == 4) { 
       float hue = float(EntityID % 100) / 100.0;
       FragColor = vec4(hue, 1.0 - hue, 0.5, 1.0); 
       return; 
    }

    vec3 V = normalize(camera.viewPos.xyz - FragPos);
    
    vec4 PBRParams = texture(gPBRParams, TexCoords);
    float Metallic = PBRParams.r;
    float Roughness = PBRParams.g;
    float Reflectivity = PBRParams.b;
    float FresnelPower = PBRParams.a;

    vec3 Lo = vec3(0.0);

    for(int i = 0; i < light.numDirLights; ++i)
    {
        vec3 L = normalize(-dirLights[i].direction);
        vec3 H = normalize(V + L);
        
        float diff = max(dot(Normal, L), 0.0);
        float spec = pow(max(dot(Normal, H), 0.0), 32.0);
        
        float shadow = 0.0;
        int sIdx = int(dirLights[i].shadowIndex);
        if (light.u_ReceiveShadow != 0 && sIdx >= 0 && sIdx < 16)
        {
            vec4 fragPosLightSpace = light.lightSpaceMatricesDir[sIdx] * vec4(FragPos, 1.0);
            shadow = ShadowCalculationDir(fragPosLightSpace, sIdx);
        }

        vec3 radiance = dirLights[i].color * dirLights[i].intensity;
        vec3 ambient  = dirLights[i].ambient * radiance * Albedo;
        vec3 diffuse  = dirLights[i].diffuse * radiance * diff * Albedo;
        vec3 specular = dirLights[i].specular * radiance * spec * 0.5; 
        
        Lo += ambient + (1.0 - shadow) * (diffuse + specular);
    }
    
    for(int i = 0; i < light.nrPointLights; ++i)
    {
        vec3 L = normalize(pointLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float dist = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * dist + pointLights[i].quadratic * (dist * dist));
        
        float diff = max(dot(Normal, L), 0.0);
        float spec = pow(max(dot(Normal, H), 0.0), 32.0);
        
        float shadow = 0.0;
        int sIdx = int(pointLights[i].shadowIndex);
        if (light.u_ReceiveShadow != 0 && sIdx >= 0 && sIdx < 16)
        {
            shadow = ShadowCalculationPoint(FragPos, pointLights[i].position, sIdx);
        }

        vec3 radiance = pointLights[i].color * pointLights[i].intensity * attenuation;
        vec3 ambient  = pointLights[i].ambient * radiance * Albedo;
        vec3 diffuse  = pointLights[i].diffuse * radiance * diff * Albedo;
        vec3 specular = pointLights[i].specular * radiance * spec * 0.5;
        
        Lo += ambient + (1.0 - shadow) * (diffuse + specular);
    }

    for(int i = 0; i < light.nrSpotLights; ++i)
    {
        vec3 L = normalize(spotLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float dist = length(spotLights[i].position - FragPos);
        float attenuation = 1.0 / (spotLights[i].constant + spotLights[i].linear * dist + spotLights[i].quadratic * (dist * dist));
        
        float theta = dot(L, normalize(-spotLights[i].direction));
        float epsilon = spotLights[i].cutOff - spotLights[i].outerCutOff;
        float spotIntensity = clamp((theta - spotLights[i].outerCutOff) / epsilon, 0.0, 1.0);
        
        float diff = max(dot(Normal, L), 0.0);
        float spec = pow(max(dot(Normal, H), 0.0), 32.0);
        
        float shadow = 0.0;
        int sIdx = int(spotLights[i].shadowIndex);
        if (light.u_ReceiveShadow != 0 && sIdx >= 0 && sIdx < 16)
        {
            vec4 fragPosLightSpace = light.lightSpaceMatricesSpot[sIdx] * vec4(FragPos, 1.0);
            shadow = ShadowCalculationSpot(fragPosLightSpace, sIdx);
        }

        vec3 radiance = spotLights[i].color * spotLights[i].intensity * attenuation * spotIntensity;
        vec3 ambient  = spotLights[i].ambient * radiance * Albedo;
        vec3 diffuse  = spotLights[i].diffuse * radiance * diff * Albedo;
        vec3 specular = spotLights[i].specular * radiance * spec * 0.5;
        
        Lo += ambient + (1.0 - shadow) * (diffuse + specular);
    }

    vec3 emissive = texture(gEmissive, TexCoords).rgb;
    
    vec3 reflectionColor = vec3(0.0);
    if (u_ProbeCount > 0 && Reflectivity > 0.01) {
        vec3 R = reflect(-V, Normal);
        vec3 finalReflection = vec3(0.0);
        float totalWeight = 0.0;
        
        for (int i = 0; i < u_ProbeCount; ++i) {
            vec3 minEdge = FragPos - u_Probes[i].boxMin;
            vec3 maxEdge = u_Probes[i].boxMax - FragPos;
            vec3 distToEdge = min(minEdge, maxEdge);
            float weight = min(distToEdge.x, min(distToEdge.y, distToEdge.z));
            
            if (weight > 0.0) {
                weight = clamp(weight, 0.0, 1.0);
                vec3 projectedR = BoxProjection(R, FragPos, u_Probes[i].pos, u_Probes[i].boxMin, u_Probes[i].boxMax);
                if (dot(projectedR, projectedR) > 0.0) {
                    vec3 sampleCol = textureLod(reflectionProbes[i], projectedR, Roughness * 5.0).rgb;
                    finalReflection += sampleCol * weight;
                    totalWeight += weight;
                }
            }
        }
        
        if (totalWeight > 0.0) {
            reflectionColor = finalReflection / totalWeight;
        } else {
            vec3 projectedR = BoxProjection(R, FragPos, u_Probes[0].pos, u_Probes[0].boxMin, u_Probes[0].boxMax);
            reflectionColor = textureLod(reflectionProbes[0], projectedR, Roughness * 5.0).rgb;
        }

        vec3 F0 = vec3(0.04); 
        // Enhance reflection saturation/vibrancy for dielectrics
        F0 = mix(F0, Albedo, max(Metallic, Reflectivity * 0.2));
        float cosTheta = max(dot(Normal, V), 0.0);
        vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, max(FresnelPower, 1.0));
        reflectionColor *= F * Reflectivity * 1.5;
    }

    vec3 ambientDiffuse = Albedo * 0.1;
    if (u_HasLightProbe) {
        ambientDiffuse = EvaluateSH(Normal) * Albedo * u_LightProbeIntensity;
    }

    vec3 color = Lo + ambientDiffuse + emissive + reflectionColor;
    FragColor = vec4(color, 1.0);
}

float ShadowCalculationDir(vec4 fragPosLightSpace, int lightIdx) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    float currentDepth = projCoords.z;
    float bias = u_ShadowBias;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMapDir, 0).xy;
    for(int x = -u_ShadowSoftness; x <= u_ShadowSoftness; ++x) {
        for(int y = -u_ShadowSoftness; y <= u_ShadowSoftness; ++y) {
            float pcfDepth = texture(shadowMapDir, vec3(projCoords.xy + vec2(x, y) * texelSize, lightIdx)).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= float((u_ShadowSoftness * 2 + 1) * (u_ShadowSoftness * 2 + 1));
    return shadow;
}

float ShadowCalculationPoint(vec3 fragPos, vec3 lightPos, int lightIdx) {
    vec3 fragToLight = fragPos - lightPos;
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
    float viewDistance = length(camera.viewPos.xyz - fragPos);
    float diskRadius = (1.0 + (viewDistance / light.farPlanePoint)) / 25.0;
    for(int i = 0; i < samples; ++i) {
        float closestDepth = texture(shadowMapPoint, vec4(fragToLight + sampleOffsetDirections[i] * diskRadius, lightIdx)).r;
        closestDepth *= light.farPlanePoint;
        if(currentDepth - bias > closestDepth) shadow += 1.0;
    }
    shadow /= float(samples);
    return shadow;
}

float ShadowCalculationSpot(vec4 fragPosLightSpace, int lightIdx) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    float currentDepth = projCoords.z;
    float bias = u_ShadowBias;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMapSpot, 0).xy;
    for(int x = -u_ShadowSoftness; x <= u_ShadowSoftness; ++x) {
        for(int y = -u_ShadowSoftness; y <= u_ShadowSoftness; ++y) {
            float pcfDepth = texture(shadowMapSpot, vec3(projCoords.xy + vec2(x, y) * texelSize, lightIdx)).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= float((u_ShadowSoftness * 2 + 1) * (u_ShadowSoftness * 2 + 1));
    return shadow;
}