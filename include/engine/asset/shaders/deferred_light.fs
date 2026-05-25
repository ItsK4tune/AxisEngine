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

layout(binding = 10) uniform sampler2DArray u_ShadowMapDir;
layout(binding = 11) uniform samplerCubeArray u_ShadowMapPoint;
layout(binding = 12) uniform sampler2DArray u_ShadowMapSpot;

uniform int u_DebugMode; 
uniform float u_ShadowBias = 0.005;
uniform int u_ShadowSoftness = 1;

layout(binding = 15) uniform samplerCube reflectionProbes[4];
uniform int u_ProbeCount = 0;
struct ProbeData {
    vec3 pos;
    vec3 boxMin;
    vec3 boxMax;
    float blendDistance;
    bool boxProjection;
};
uniform ProbeData u_Probes[4];

layout(binding = 19) uniform sampler2D u_PlanarReflections[4];
uniform int u_PlanarCount;
uniform vec2 u_ScreenSize;
uniform vec3 u_PlanarNormals[4];

vec3 BoxProjection(vec3 dir, vec3 fragPos, vec3 probePos, vec3 boxMin, vec3 boxMax) {
    vec3 firstPlaneIntersect = (boxMax - fragPos) / dir;
    vec3 secondPlaneIntersect = (boxMin - fragPos) / dir;
    vec3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
    float dist = min(furthestPlane.x, min(furthestPlane.y, furthestPlane.z));
    return dir * dist + (fragPos - probePos);
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

float ShadowCalculationDir(vec4 fragPosLightSpace, int lightIdx);
float ShadowCalculationPoint(vec3 fragPos, vec3 u_LightPos, int lightIdx);
float ShadowCalculationSpot(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int lightIdx);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float u_Roughness);

void main()
{             
    vec4 normalSample = texture(gNormal, TexCoords);
    vec3 rawNormal = normalSample.rgb;
    if (dot(rawNormal, rawNormal) <= 0.000001) discard;

    // GBuffer position is authoritative; depth may intentionally stay unchanged for ignoreDepth overlays.
    vec3 FragPos = texture(gPosition, TexCoords).xyz;
    if (any(isnan(FragPos)) || any(isinf(FragPos))) discard;

    vec3 Normal = normalize(rawNormal);
    bool receiveShadowFlag = (normalSample.a > 0.5);
    
    vec4 albedoSpecSample = texture(gAlbedoSpec, TexCoords);
    vec3 Albedo = albedoSpecSample.rgb;
    float FresnelBias = albedoSpecSample.a; // Unpacked from deferred_reflect.fs
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
    float Metallic = clamp(PBRParams.r, 0.0, 1.0);
    float Roughness = clamp(PBRParams.g, 0.04, 1.0);
    float Reflectivity = PBRParams.b;
    
    // Unpacking: Integer part = ProbeIndex + 1, Fractional part = FresnelPower / 100.0
    int pIdx = int(PBRParams.a) - 1;
    float FresnelPower = fract(PBRParams.a) * 100.0;

    // Mask out specular lighting on pure matte surfaces (e.g. black decals, cracks)
    float activeSpecularMask = 1.0; 
    if (Reflectivity < 0.01 && Roughness > 0.99) activeSpecularMask = 0.0;

    vec3 F0Direct = mix(vec3(0.04), Albedo, Metallic);
    vec3 Lo = vec3(0.0);

    for(int i = 0; i < light.numDirLights; ++i)
    {
        vec3 L = normalize(-dirLights[i].direction);
        vec3 H = normalize(V + L);
        
        float diff = max(dot(Normal, L), 0.0);
        float specPower = mix(128.0, 2.0, Roughness);
        float spec = pow(max(dot(Normal, H), 0.0), specPower) * (1.0 - Roughness);
        
        float shadow = 0.0;
        int sIdx = int(dirLights[i].shadowIndex);
        if (light.u_ReceiveShadow != 0 && receiveShadowFlag && sIdx >= 0 && sIdx < 16)
        {
            vec4 fragPosLightSpace = light.lightSpaceMatricesDir[sIdx] * vec4(FragPos, 1.0);
            shadow = ShadowCalculationDir(fragPosLightSpace, sIdx);
        }

        vec3 F = fresnelSchlickRoughness(max(dot(H, V), 0.0), F0Direct, Roughness);
        vec3 kD = (vec3(1.0) - F) * (1.0 - Metallic);
        vec3 radiance = dirLights[i].color * dirLights[i].intensity;
        vec3 diffuse  = dirLights[i].diffuse * radiance * diff * Albedo * kD;
        vec3 u_Specular = dirLights[i].u_Specular * radiance * spec * F * activeSpecularMask;
        
        Lo += (1.0 - shadow) * (diffuse + u_Specular);
    }
    
    for(int i = 0; i < light.nrPointLights; ++i)
    {
        vec3 L = normalize(pointLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float dist = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * dist + pointLights[i].quadratic * (dist * dist));
        
        float diff = max(dot(Normal, L), 0.0);
        float specPower = mix(128.0, 2.0, Roughness);
        float spec = pow(max(dot(Normal, H), 0.0), specPower) * (1.0 - Roughness);
        
        float shadow = 0.0;
        int sIdx = int(pointLights[i].shadowIndex);
        if (light.u_ReceiveShadow != 0 && receiveShadowFlag && sIdx >= 0 && sIdx < 16)
        {
            shadow = ShadowCalculationPoint(FragPos, pointLights[i].position, sIdx);
        }

        vec3 F = fresnelSchlickRoughness(max(dot(H, V), 0.0), F0Direct, Roughness);
        vec3 kD = (vec3(1.0) - F) * (1.0 - Metallic);
        vec3 radiance = pointLights[i].color * pointLights[i].intensity * attenuation;
        vec3 diffuse  = pointLights[i].diffuse * radiance * diff * Albedo * kD;
        vec3 u_Specular = pointLights[i].u_Specular * radiance * spec * F * activeSpecularMask;
        
        Lo += (1.0 - shadow) * (diffuse + u_Specular);
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
        float specPower = mix(128.0, 2.0, Roughness);
        float spec = pow(max(dot(Normal, H), 0.0), specPower) * (1.0 - Roughness);
        
        float shadow = 0.0;
        int sIdx = int(spotLights[i].shadowIndex);
        if (light.u_ReceiveShadow != 0 && receiveShadowFlag && sIdx >= 0 && sIdx < 16)
        {
            vec4 fragPosLightSpace = light.lightSpaceMatricesSpot[sIdx] * vec4(FragPos, 1.0);
            shadow = ShadowCalculationSpot(fragPosLightSpace, Normal, L, sIdx);
        }

        vec3 F = fresnelSchlickRoughness(max(dot(H, V), 0.0), F0Direct, Roughness);
        vec3 kD = (vec3(1.0) - F) * (1.0 - Metallic);
        vec3 radiance = spotLights[i].color * spotLights[i].intensity * attenuation * spotIntensity;
        vec3 diffuse  = spotLights[i].diffuse * radiance * diff * Albedo * kD;
        vec3 u_Specular = spotLights[i].u_Specular * radiance * spec * F * activeSpecularMask;
        
        Lo += (1.0 - shadow) * (diffuse + u_Specular);
    }

    vec3 emissive = texture(gEmissive, TexCoords).rgb;
    
    vec3 ambientDiffuse = Albedo * 0.15 * (1.0 - Metallic * 0.75);
    vec3 ambientSpecular = F0Direct * 0.15 * Metallic;
    if (u_HasLightProbe) {
        ambientDiffuse = EvaluateSH(Normal) * Albedo * u_LightProbeIntensity;
        ambientSpecular = vec3(0.0);
    }

    vec3 reflectionColor = vec3(0.0);
    if (u_ProbeCount > 0 && Reflectivity > 0.01) {
        vec3 R = reflect(-V, Normal);
        
        // 1. Mandatory Probe Override from G-Buffer
        if (pIdx >= 0 && pIdx < u_ProbeCount) {
            vec3 L = (u_Probes[pIdx].boxProjection) ? 
                        BoxProjection(R, FragPos, u_Probes[pIdx].pos, u_Probes[pIdx].boxMin, u_Probes[pIdx].boxMax) : R;
            reflectionColor = textureLod(reflectionProbes[pIdx], L, Roughness * 2.0).rgb;
        } 
        else {
            // 2. Fallback to Automatic Spatial Blending Logic
            vec3 finalLocalReflection = vec3(0.0);
            float totalWeight = 0.0;
            float maxBoxWeight = 0.0;
            
            for (int i = 0; i < u_ProbeCount; ++i) {
                vec3 minEdge = FragPos - u_Probes[i].boxMin;
                vec3 maxEdge = u_Probes[i].boxMax - FragPos;
                vec3 distToEdge = min(minEdge, maxEdge);
                float weight = min(distToEdge.x, min(distToEdge.y, distToEdge.z));
                
                float boxWeight = smoothstep(0.0, 1.0, weight / max(u_Probes[i].blendDistance, 0.01));
                maxBoxWeight = max(maxBoxWeight, boxWeight);

                if (boxWeight > 0.0) {
                    vec3 boxSize = u_Probes[i].boxMax - u_Probes[i].boxMin;
                    float volume = boxSize.x * boxSize.y * boxSize.z;
                    float distSq = dot(FragPos - u_Probes[i].pos, FragPos - u_Probes[i].pos);
                    float priority = (1.0 / (volume + 0.0001)) / (distSq + 0.0001);
                    float finalWeight = pow(boxWeight, 4.0) * priority;

                    vec3 L = (u_Probes[i].boxProjection) ? 
                            BoxProjection(R, FragPos, u_Probes[i].pos, u_Probes[i].boxMin, u_Probes[i].boxMax) : R;

                    if (dot(L, L) > 0.0) {
                        finalLocalReflection += textureLod(reflectionProbes[i], L, Roughness * 2.0).rgb * finalWeight;
                        totalWeight += finalWeight;
                    }
                }
            }
            
            vec3 globalFallback = textureLod(reflectionProbes[0], R, Roughness * 2.0).rgb;
            vec3 combinedLocal = (totalWeight > 0.000001) ? (finalLocalReflection / totalWeight) : globalFallback;
            reflectionColor = mix(globalFallback, combinedLocal, maxBoxWeight);
        }
    }
    
    if (u_PlanarCount > 0 && Reflectivity > 0.01) {
        vec3 bestPlanarColor = vec3(0.0);
        float bestPlanarMask = 0.0;
        
        vec2 uv = gl_FragCoord.xy / u_ScreenSize;
        
        for (int i = 0; i < u_PlanarCount; ++i) {
            vec3 activeNormal = u_PlanarNormals[i];
            float cosTheta = abs(dot(Normal, activeNormal));
            
            // Mask for this specific mirror plane
            float mask = clamp((cosTheta - 0.98) * 50.0, 0.0, 1.0);
            
            if (mask > 0.0) {
                // Centered distortion using the specific plane normal
                vec3 dN = Normal - activeNormal;
                vec3 side = normalize(cross(activeNormal, abs(activeNormal.y) > 0.9 ? vec3(1,0,0) : vec3(0,1,0)));
                vec3 up = cross(side, activeNormal);
                vec2 distortion = vec2(dot(dN, side), dot(dN, up)) * Roughness * 0.1;
                
                vec2 uv = TexCoords + distortion;
                vec3 sampleColor = texture(u_PlanarReflections[i], clamp(uv, 0.0, 1.0)).rgb;
                
                // Alpha-based accumulation for overlapping mirrors or transitions
                bestPlanarColor += sampleColor * mask;
                bestPlanarMask = max(bestPlanarMask, mask);
            }
        }
        
        if (bestPlanarMask > 0.0) {
            reflectionColor = mix(reflectionColor, bestPlanarColor, bestPlanarMask);
        }
    }

    if (Reflectivity > 0.01) {
        vec3 F0 = vec3(max(FresnelBias, 0.04)); // Use per-object FresnelBias instead of hardcoded 0.08
        F0 = mix(F0, Albedo, Metallic);
        vec3 F = fresnelSchlickRoughness(max(dot(Normal, V), 0.0), F0, Roughness);
        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - Metallic);
        
        reflectionColor *= F * Reflectivity * 1.5 * activeSpecularMask; 
        ambientDiffuse *= kD;
    }

    vec3 color = Lo + ambientDiffuse + emissive + reflectionColor;
    FragColor = vec4(color, 1.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float u_Roughness) {
    return F0 + (max(vec3(1.0 - u_Roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ShadowCalculationDir(vec4 fragPosLightSpace, int lightIdx) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    if (projCoords.z > 1.0) return 0.0;
    float currentDepth = projCoords.z;
    float bias = max(u_ShadowBias, 0.0025);
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
