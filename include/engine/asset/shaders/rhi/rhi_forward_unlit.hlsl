struct PushConstants
{
    float4x4 u_Mvp;
    float4 u_Color;
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
    float4 color : COLOR0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), pc.u_Mvp);
    output.color = pc.u_Color;
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
    float3 color = input.color.rgb;
    color = ACESFilm(color);
    color = pow(color, 1.0f / 2.2f);
    return float4(color, input.color.a);
}
