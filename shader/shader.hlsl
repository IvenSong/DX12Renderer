cbuffer TransformConstants : register(b0)
{
    row_major float4x4 MVP;
    row_major float4x4 normalMatrix;
    row_major float4x4 lightMVP;
};

cbuffer LightBuffer : register(b1)
{
    float3 lightDirection;
    float diffuseIntensity;

    float3 lightColor;
    float ambientIntensity;
};


Texture2D diffuseTexture : register(t0);

Texture2D<float> shadowMap : register(t1);

SamplerState textureSampler : register(s0);

SamplerComparisonState shadowSampler : register(s1);


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
    float4 lightSpacePos : TEXCOORD1;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.position = mul(float4(input.position, 1.0f), MVP);

    output.color = input.color;
    output.uv = input.uv;

    output.normal = mul(input.normal, (float3x3)normalMatrix);

    output.lightSpacePos = mul(float4(input.position, 1.0f), lightMVP);
    
    return output;
}

float4 ShadowVSMain(VSInput input) : SV_POSITION
{
    return mul(float4(input.position, 1.0f), lightMVP);
}

float CalculateShadow(float4 lightSpacePos)
{
    // 1. 透视除法（正交投影 w=1，但保持通用写法）
    float3 proj = lightSpacePos.xyz / lightSpacePos.w;
    // 2. NDC [-1, 1] 映射到纹理 UV [0, 1]（注意 Y 要翻转）
    float2 uv = proj.xy * float2(0.5f, -0.5f) + 0.5f;
    // 3. 超出阴影贴图范围的区域直接视为不被遮挡
    if (any(uv < 0.0f) || any(uv > 1.0f) || proj.z < 0.0f || proj.z > 1.0f)
        return 1.0f;
    // 4. 硬件比较采样（加小偏移防止地面自己把自己挡住产生条纹）
    float bias = 0.005f;
    return shadowMap.SampleCmpLevelZero(shadowSampler, uv, proj.z - bias);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 textureColor = diffuseTexture.Sample(textureSampler, input.uv).rgb;
    float4 baseColor = float4(input.color.rgb, input.color.a);
    
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(-lightDirection);

    float3 diffuse = max(dot(normal, lightDir), 0.0f) * diffuseIntensity * lightColor;

    float3 ambient = baseColor.rgb * ambientIntensity;

    float shadow = CalculateShadow(input.lightSpacePos);
    return float4(ambient + diffuse * shadow * baseColor.rgb, baseColor.a);
}
