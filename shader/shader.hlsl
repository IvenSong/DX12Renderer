cbuffer TransformConstants : register(b0)
{
    row_major float4x4 MVP;
};

cbuffer LightBuffer : register(b1)
{
    float3 lightDirection;
    float diffuseIntensity;

    float3 lightColor;
    float ambientIntensity;
};


Texture2D diffuseTexture : register(t0);

SamplerState textureSampler : register(s0);

RWTexture2D<float4> outputTexture : register(u0);


struct VSInput {
	float3 position : POSITION;
	float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.position = mul(float4(input.position, 1.0f), MVP);

    output.color = input.color;
    output.uv = input.uv;

    output.normal = mul(float4(input.normal, 0.0f), MVP).xyz;

    return output;
}


float4 PSMain(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(-lightDirection);

    float diffuse = max(dot(normal, lightDir), 0.0f);

    float3 ambient = input.color * ambientIntensity;

    return float4(ambient + diffuse * input.color.xyz, input.color.a);
}
