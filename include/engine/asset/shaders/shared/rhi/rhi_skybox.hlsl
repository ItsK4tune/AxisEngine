struct PushConstants
{
    float4x4 u_Mvp;
    float u_Intensity;
    float3 pad;
};

#if defined(AXIS_VULKAN)
[[vk::push_constant]] PushConstants pc;
#else
ConstantBuffer<PushConstants> pc : register(b255, space0);
#endif

struct VSInput
{
    float3 position : ATTRIBUTE0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 texCoords : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    float4 pos = mul(float4(input.position, 1.0f), pc.u_Mvp);
    output.position = pos.xyww;
    output.texCoords = input.position;
    return output;
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
    float3 dir = normalize(input.texCoords);
    float3 skyColor = lerp(float3(0.1f, 0.2f, 0.4f), float3(0.4f, 0.6f, 0.9f),
                           clamp(dir.y * 0.5f + 0.5f, 0.0f, 1.0f));
    float3 color = skyColor * pc.u_Intensity;
    color = ACESFilm(color);
    color = pow(color, 1.0f / 2.2f);
    return float4(color, 1.0f);
}
