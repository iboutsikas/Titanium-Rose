#define MyRS1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
                        "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
                        "DENY_HULL_SHADER_ROOT_ACCESS | " \
                        "DENY_GEOMETRY_SHADER_ROOT_ACCESS), " \
                "CBV(b0)," \
                "CBV(b1)," \
                "StaticSampler(s0," \
                            "filter = FILTER_MIN_MAG_MIP_POINT," \
                            "addressU = TEXTURE_ADDRESS_BORDER, " \
                            "addressV = TEXTURE_ADDRESS_BORDER, " \
                            "addressW = TEXTURE_ADDRESS_BORDER, " \
                            "mipLODBias = 0.f, " \
                            "maxAnisotropy = 0, " \
                            "comparisonFunc = COMPARISON_NEVER, " \
                            "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK, " \
                            "minLOD = 0.f, " \
                            "maxLOD = 3.402823466e+38f, " \
                            "space = 0," \
                            "visibility = SHADER_VISIBILITY_PIXEL)," \
                "DescriptorTable(SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL) " 

cbuffer cbPass : register(b0) {
    matrix gViewProjection;
    float4 gAmbientLight;
    float4 gDirectionalLight;
    float3 gCameraPosition;
    float  gAmbientIntensity;
};

cbuffer cbPerObject : register(b1) {
    matrix oLocalToWorld;
    matrix oNormalsMatrix;
    uint   oTextureIndex;
}

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 uv: UV;
    float4 worldPosition : POSITION;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal: NORMAL;
    float2 uv: UV;
};


[RootSignature(MyRS1)]
PSInput VS_Main(VSInput input)
{
    PSInput result;
    float2 vUv = input.uv;
    vUv = (vUv * 2.0) - 1.0;
    
    matrix mvp = mul(gViewProjection, oLocalToWorld);

    result.position = float4(vUv, 1.0, 1.0);
    result.worldPosition = mul(mvp, float4(input.position, 1.0));
    result.normal = mul(oNormalsMatrix, float4(input.normal, 0.0)).xyz;
    result.color = input.color;
    result.uv = input.uv;
    result.uv.y = 1.0 - result.uv.y;

    return result;
}

[RootSignature(MyRS1)]
float4 PS_Main(PSInput input) : SV_TARGET
{
    float4 ambient = gAmbientLight * gAmbientIntensity;

    float3 l = normalize(gCameraPosition - input.worldPosition.xyz);
    float3 normal = normalize(input.normal);
    float brightness = max(dot(normal, l), 0.0f);

    float4 texColor = g_texture.Sample(g_sampler, input.uv);

    // if (texColor.r != 0.0 || texColor.b != 0.0 || texColor.g != 0.0) {
    //     texColor = float4(1.0, 1.0, 1.0, 1.0);
    // }
    
    float4 diffuseColor = gDirectionalLight * brightness * texColor;

    return  (ambient * texColor) + diffuseColor;
    // return texColor;
}