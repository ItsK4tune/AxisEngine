#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

// 1. G-Buffer Input Samplers (Standardized Binding 0-3)
layout (binding = 0) uniform sampler2D gPosition;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D gAlbedoSpec;
layout (binding = 3) uniform usampler2D gID;

// 2. UBO/SSBO Bindings (Standardized Binding 20-25)
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

// 3. Shadow Maps (Standardized Binding 10-15)
layout (binding = 10) uniform sampler2D shadowMapDir[2];
layout (binding = 12) uniform samplerCube shadowMapPoint[2];
layout (binding = 14) uniform sampler2D shadowMapSpot[2];

uniform int u_DebugMode; 

float ShadowCalculationDir(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex);

void main()
{             
    // 1. Retrieve data from G-buffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    if (dot(FragPos, FragPos) < 0.0001) discard; // Optimization: early discard for background

    vec3 Normal = texture(gNormal, TexCoords).rgb;
    Normal = normalize(Normal * 2.0 - 1.0); // Decode from [0, 1]
    
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Roughness = texture(gAlbedoSpec, TexCoords).a;
    uint EntityID = texture(gID, TexCoords).r;

    // 2. Debug Views
    if (u_DebugMode == 1) { FragColor = vec4(FragPos, 1.0); return; }
    if (u_DebugMode == 2) { FragColor = vec4(Normal * 0.5 + 0.5, 1.0); return; }
    if (u_DebugMode == 3) { FragColor = vec4(Albedo, 1.0); return; }
    if (u_DebugMode == 4) { 
       float hue = float(EntityID % 100) / 100.0;
       FragColor = vec4(hue, 1.0 - hue, 0.5, 1.0); 
       return; 
    }

    // 3. Lighting
    vec3 V = normalize(camera.viewPos - FragPos);
    vec3 Lo = vec3(0.0);

    // Directional Lights
    for(int i = 0; i < light.numDirLights; ++i)
    {
        vec3 L = normalize(-dirLights[i].direction);
        vec3 H = normalize(V + L);
        
        float diff = max(dot(Normal, L), 0.0);
        float spec = pow(max(dot(Normal, H), 0.0), 32.0); // Simplified shininess
        
        float shadow = 0.0;
        int sIdx = int(dirLights[i].shadowIndex);
        if (light.u_ReceiveShadow != 0 && sIdx >= 0 && sIdx < 2)
        {
            vec4 fragPosLightSpace = light.lightSpaceMatricesDir[sIdx] * vec4(FragPos, 1.0);
            shadow = ShadowCalculationDir(fragPosLightSpace, Normal, L, sIdx);
        }

        vec3 radiance = dirLights[i].color * dirLights[i].intensity;
        
        vec3 ambient  = 0.1 * radiance * Albedo;
        vec3 diffuse  = 0.8 * radiance * diff * Albedo;
        vec3 specular = 1.0 * radiance * spec * 0.5; // 0.5 as default spec intensity
        
        Lo += ambient + (1.0 - shadow) * (diffuse + specular);
    }

    // Final Color
    vec3 color = Lo + Albedo * 0.01; // Tiny global ambient for deep shadows
    
    FragColor = vec4(color, 1.0);
}

float ShadowCalculationDir(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, int shadowMapIndex)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    
    float closestDepth = texture(shadowMapDir[shadowMapIndex], projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMapDir[shadowMapIndex], 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMapDir[shadowMapIndex], projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}
