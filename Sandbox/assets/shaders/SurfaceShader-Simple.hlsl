#define MyRS1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
                        "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
                        "DENY_HULL_SHADER_ROOT_ACCESS | " \
                        "DENY_GEOMETRY_SHADER_ROOT_ACCESS), " \
              "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL)," \
              "DescriptorTable(UAV(u0), visibility = SHADER_VISIBILITY_PIXEL)," \
              "CBV(b0)," \
              "CBV(b1)," \
              "StaticSampler(s0," \
                      "addressU = TEXTURE_ADDRESS_BORDER," \
                      "addressV = TEXTURE_ADDRESS_BORDER," \
                      "addressW = TEXTURE_ADDRESS_BORDER," \
                      "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK," \
                      "filter = FILTER_ANISOTROPIC, "\
                      "visibility = SHADER_VISIBILITY_PIXEL)"

#include "Common.hlsli"

cbuffer cbPass : register(b1) {
    matrix ViewProjection;
};

cbuffer cbPerObject : register(b0) {
    matrix  LocalToWorld;
    uint2   FeedbackDims;
    uint2   _padding;
}

Texture2D ColorTexture : register(t0);
RWBuffer<uint> FeedbackBuffer: register(u0);
SamplerState g_sampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv: UV;
};

[RootSignature(MyRS1)]
PSInput VS_Main(VSInput input)
{
    PSInput result;

    matrix mvp = mul(ViewProjection, LocalToWorld);

    result.position = mul(mvp, float4(input.position, 1.0f));
    result.uv = input.uv;
    result.uv.y = 1.0 - result.uv.y;
    
    return result;
}

[RootSignature(MyRS1)]
float4 PS_Main(PSInput input) : SV_TARGET
{
    uint idealMipLevel = ColorTexture.CalculateLevelOfDetail(g_sampler, input.uv);
    
    uint feedbackX = (uint)((float)FeedbackDims.x * input.uv.x) ;
    uint feedbackY = (uint)((float)FeedbackDims.y * input.uv.y);

    uint index = feedbackY * FeedbackDims.x + feedbackX;
    InterlockedMin(FeedbackBuffer[index], idealMipLevel);

    // return MipDebugColor(idealMipLevel);
    // return float4(idealMipLevel / 10, 0, 0, 1);
    // float4 smpl = ColorTexture.SampleLevel(g_sampler, input.uv, 3);
    float4 smpl = ColorTexture.Sample(g_sampler, input.uv);


    // if (smpl.a == 0) {
    //     return float4(1, 0, 1, 1);
    // }
    return smpl;
}