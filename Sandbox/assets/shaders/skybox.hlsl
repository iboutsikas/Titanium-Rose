#include "RootSignatures.hlsli"

struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 eyeDirection: EYE_DIR;
};

cbuffer cbPass: register(b0)
{
    matrix ViewInverse;
    matrix ProjInverse;
    uint MipLevel;
}

TextureCube skyMap : register(t0);
SamplerState skySampler: register(s0);

[RootSignature(Skybox_RS)]
PSInput VS_Main(VSInput vin)
{
    PSInput vout;

    vout.position = float4(vin.position.xy, 1.0, 1.0);
    float3 unprojected = mul(ProjInverse, vout.position).xyz;
    vout.eyeDirection = mul((float3x3)ViewInverse, unprojected).xyz;
    return vout;
}

[RootSignature(Skybox_RS)]
float4 PS_Main(PSInput pin) : SV_Target
{

    return skyMap.SampleLevel(skySampler, pin.eyeDirection, MipLevel);
}

