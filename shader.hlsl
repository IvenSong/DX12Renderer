cbuffer TransformConstants : register(b0)
{
    row_major float4x4 MVP;
};

struct VSInput {
	float3 position : POSITION;
	float3 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.position = mul(float4(input.position, 1.0f), MVP);
    output.color = input.color;

    return output;
}


float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}