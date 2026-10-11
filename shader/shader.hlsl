cbuffer TransformConstants : register(b0)
{
    row_major float4x4 MVP;
};


Texture2D diffuseTexture : register(t0);

SamplerState textureSampler : register(s0);

RWTexture2D<float4> outputTexture : register(u0);


struct VSInput {
	float3 position : POSITION;
	float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.position = mul(float4(input.position, 1.0f), MVP);
    output.color = input.color;
    output.uv = input.uv;

    return output;
}


float4 PSMain(PSInput input) : SV_TARGET
{
    // UV debug: U controls red, V controls green. Edit the vertex UVs to test.
    return float4(input.uv, 1.0f, 1.0f);
}
