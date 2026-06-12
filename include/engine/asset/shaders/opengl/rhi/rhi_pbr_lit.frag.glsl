#version 460 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

uniform vec4 u_Color;
uniform vec4 u_PbrParams;
uniform vec4 u_CameraPos;
uniform vec4 u_DirLightDir;
uniform vec4 u_DirLightColor;

layout(location = 0) out vec4 FragColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) *
           GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ACESFilm(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 albedo = u_Color.rgb;
    float roughness = clamp(u_PbrParams.x, 0.04, 1.0);
    float metallic = clamp(u_PbrParams.y, 0.0, 1.0);
    float ao = u_PbrParams.z;

    vec3 N = normalize(v_Normal);
    vec3 V = normalize(u_CameraPos.xyz - v_FragPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 Lo = vec3(0.0);

    if (u_DirLightDir.w > 0.0)
    {
        vec3 L = normalize(-u_DirLightDir.xyz);
        vec3 H = normalize(V + L);
        vec3 radiance = u_DirLightColor.rgb * u_DirLightColor.w;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 specular = (NDF * G * F) /
                        (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001);

        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        Lo += (kD * albedo / PI + specular) * radiance * max(dot(N, L), 0.0);
    }

    vec3 color = albedo * 0.03 * ao + Lo;
    color = ACESFilm(color);
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, u_Color.a);
}
