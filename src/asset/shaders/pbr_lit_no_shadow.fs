#version 430 core
out vec4 FragColor;



in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_metallic1;
    sampler2D texture_roughness1;
    sampler2D texture_ao1;
    sampler2D texture_normal1;
    sampler2D texture_emission1;

    float roughness;
    float metallic;
    float ao;
    vec3 emission;
    float opacity;
};

struct DirLight {
    vec3 direction; float pad0;
    vec3 color; float intensity;
    vec3 ambient; float pad1;
    vec3 diffuse; float pad2;
    vec3 specular; float pad3;
};

struct PointLight {
    vec3 position; float pad0;
    vec3 color; float intensity;
    float constant; float linear; float quadratic; float radius;
    vec3 ambient; float pad1;
    vec3 diffuse; float pad2;
    vec3 specular; float pad3;
};

struct SpotLight {
    vec3 position; float pad0;
    vec3 direction; float pad1;
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

uniform int numDirLights;
uniform int nrPointLights;
uniform int nrSpotLights;

// uniform DirLight dirLight;
// uniform DirLight dirLights[NR_DIR_LIGHTS];
// uniform PointLight pointLights[NR_POINT_LIGHTS];

uniform vec3 viewPos;
uniform Material material;
uniform vec4 tintColor;
uniform bool debug_noTexture;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

void main()
{
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;

    if (debug_noTexture)
    {
        albedo = vec3(1.0) * tintColor.rgb; // White clay
        metallic = 0.0;
        roughness = 0.8;
        ao = 1.0;
    }
    else
    {
        albedo = pow(texture(material.texture_diffuse1, TexCoords).rgb, vec3(2.2)) * tintColor.rgb;
        metallic = texture(material.texture_metallic1, TexCoords).r * material.metallic;
        roughness = texture(material.texture_roughness1, TexCoords).r * material.roughness;
        ao = texture(material.texture_ao1, TexCoords).r * material.ao;
    }

    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    if (numDirLights > 0) {
        for(int d = 0; d < numDirLights; d++) {
            vec3 L = normalize(-dirLights[d].direction);
            vec3 H = normalize(V + L);

            vec3 radiance = dirLights[d].color * dirLights[d].intensity;

            float NDF = DistributionGGX(N, H, roughness);
            float G   = GeometrySmith(N, V, L, roughness);
            vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 specular = numerator / denominator;

            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;

            float NdotL = max(dot(N, L), 0.0);
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        }
    }

    for(int i = 0; i < nrPointLights; ++i)
    {
        vec3 L = normalize(pointLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float distance = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * (distance * distance));
        vec3 radiance = pointLights[i].color * pointLights[i].intensity * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Spot Lights for PBR
    for(int i = 0; i < nrSpotLights; ++i)
    {
        vec3 L = normalize(spotLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float distance = length(spotLights[i].position - FragPos);
        float attenuation = 1.0 / (spotLights[i].constant + spotLights[i].linear * distance + spotLights[i].quadratic * (distance * distance));
        
        vec3 lightDir = normalize(spotLights[i].position - FragPos);
        float theta = dot(lightDir, normalize(-spotLights[i].direction));
        float epsilon = spotLights[i].cutOff - spotLights[i].outerCutOff;
        float intensity = clamp((theta - spotLights[i].outerCutOff) / epsilon, 0.0, 1.0);
        
        vec3 radiance = spotLights[i].color * spotLights[i].intensity * attenuation * intensity;

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    vec3 emission = texture(material.texture_emission1, TexCoords).rgb + material.emission;
    color += emission;

    if (material.opacity < 0.1) discard;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, material.opacity) * tintColor.a;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
