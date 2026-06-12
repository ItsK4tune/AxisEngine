struct PushConstants
{
    float4x4 u_Mvp;
    float4x4 u_Model;
    float4 u_Color;
    float4 u_PbrParams;
    float4 u_CameraPos;
    float4 u_DirLightDir;
    float4 u_DirLightColor;
};

#if defined(AXIS_VULKAN)
[[vk::push_constant]] PushConstants pc;
#else
ConstantBuffer<PushConstants> pc : register(b255, space0);
#endif

struct VSInput
{
    float3 position : ATTRIBUTE0;
    float3 normal : ATTRIBUTE1;
    float2 texCoords : ATTRIBUTE2;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 fragPos : POSITION0;
    float3 normal : NORMAL0;
    float2 texCoords : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), pc.u_Mvp);
    output.fragPos = mul(float4(input.position, 1.0f), pc.u_Model).xyz;
    output.normal = mul(input.normal, (float3x3)pc.u_Model);
    output.texCoords = input.texCoords;
    return output;
}

static const float PI = 3.14159265359f;

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float denom = (NdotH * NdotH * (a2 - 1.0f) + 1.0f);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N, V), 0.0f), roughness) *
           GeometrySchlickGGX(max(dot(N, L), 0.0f), roughness);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

float4 PSMain(VSOutput input) : SV_Target0
{
    float3 albedo = pc.u_Color.rgb;
    float roughness = clamp(pc.u_PbrParams.x, 0.04f, 1.0f);
    float metallic = clamp(pc.u_PbrParams.y, 0.0f, 1.0f);
    float ao = pc.u_PbrParams.z;

    float3 N = normalize(input.normal);
    float3 V = normalize(pc.u_CameraPos.xyz - input.fragPos);
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    if (pc.u_DirLightDir.w > 0.0f)
    {
        float3 L = normalize(-pc.u_DirLightDir.xyz);
        float3 H = normalize(V + L);
        float3 radiance = pc.u_DirLightColor.rgb * pc.u_DirLightColor.w;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);
        float3 specular = (NDF * G * F) /
                          (4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f);

        float3 kD = (float3(1.0f, 1.0f, 1.0f) - F) * (1.0f - metallic);
        Lo += (kD * albedo / PI + specular) * radiance * max(dot(N, L), 0.0f);
    }

    float3 color = albedo * 0.03f * ao + Lo;
    color = ACESFilm(color);
    color = pow(color, 1.0f / 2.2f);
    return float4(color, pc.u_Color.a);
}
