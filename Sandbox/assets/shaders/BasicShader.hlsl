#define MyRS1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
                        "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
                        "DENY_HULL_SHADER_ROOT_ACCESS | " \
                        "DENY_GEOMETRY_SHADER_ROOT_ACCESS), " \
              "CBV(b0)," \
              "CBV(b1)," \
              "DescriptorTable(SRV(t0, numDescriptors = 3, flags = DESCRIPTORS_VOLATILE), visibility = SHADER_VISIBILITY_PIXEL), " \
              "StaticSampler(s0," \
                      "addressU = TEXTURE_ADDRESS_BORDER," \
                      "addressV = TEXTURE_ADDRESS_BORDER," \
                      "addressW = TEXTURE_ADDRESS_BORDER," \
                      "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK," \
                      "filter = FILTER_MIN_MAG_MIP_POINT, "\
                      "visibility = SHADER_VISIBILITY_PIXEL)"

cbuffer cbPass : register(b0) {
    matrix gViewProjection;
};

cbuffer cbPerObject : register(b1) {
    matrix oLocalToWorld;
    float4 oMaterialColor;
    uint   oTextureIndex;
}

Texture2D g_texture[3] : register(t0);
SamplerState g_sampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv: UV;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal: NORMAL;
    float2 uv: UV;
};

[RootSignature(MyRS1)]
PSInput VS_Main(VSInput input)
{
    PSInput result;

    matrix mvp = mul(gViewProjection, oLocalToWorld);

    result.position = mul(mvp, float4(input.position, 1.0f));
    result.uv = input.uv;
    result.uv.y = 1.0 - result.uv.y;
    
    return result;
}

[RootSignature(MyRS1)]
float4 PS_Main(PSInput input) : SV_TARGET
{
    // return float4(1.0, 0.0, 0.0, 1.0);
    return g_texture[oTextureIndex].Sample(g_sampler, input.uv) * oMaterialColor;
}