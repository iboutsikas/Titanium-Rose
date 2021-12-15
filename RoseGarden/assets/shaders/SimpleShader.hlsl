struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

PSInput VS_Main(VSInput input)
{
    PSInput result;

    result.position = float4(input.position, 1.0f); // mul(ModelViewProjectionCB.MVP, float4(input.position, 1.0f));
    result.color = input.color;

    return result;
}

float4 PS_Main(PSInput input) : SV_TARGET
{
    return input.color;
}