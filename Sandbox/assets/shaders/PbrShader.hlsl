#define MyRS1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
                        "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
                        "DENY_HULL_SHADER_ROOT_ACCESS | " \
                        "DENY_GEOMETRY_SHADER_ROOT_ACCESS), " \
                "CBV(b0)," \
                "CBV(b1)," \
                "DescriptorTable(SRV(t0, numDescriptors = 4, flags = DESCRIPTORS_VOLATILE), visibility = SHADER_VISIBILITY_PIXEL), " \
                "StaticSampler(s0," \
                      "addressU = TEXTURE_ADDRESS_WRAP," \
                      "addressV = TEXTURE_ADDRESS_WRAP," \
                      "addressW = TEXTURE_ADDRESS_WRAP," \
                      "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK," \
                      "filter = FILTER_ANISOTROPIC, "\
                      "visibility = SHADER_VISIBILITY_PIXEL),"\
                "StaticSampler(s1," \
                      "addressU = TEXTURE_ADDRESS_WRAP," \
                      "addressV = TEXTURE_ADDRESS_WRAP," \
                      "addressW = TEXTURE_ADDRESS_WRAP," \
                      "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK," \
                      "filter = FILTER_MIN_MAG_MIP_LINEAR, "\
                      "visibility = SHADER_VISIBILITY_PIXEL)"

#include "Common.hlsli"

cbuffer cbPass : register(b0) {
    matrix   gViewProjection;
    float4   gAmbientLight;
    float4   gDirectionalLight;
    float3   gDirectionalLightPosition;
    float    __padding1;
    float3   gCameraPosition;
    float1   gAmbientIntensity;
};

cbuffer cbPerObject : register(b1) {
    matrix oLocalToWorld;
    matrix oNormalsMatrix;
    float  oGlossiness;
};

Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D metalnessTexture: register(t2);
Texture2D roughnessTexture: register(t3);

SamplerState g_sampler : register(s0);
SamplerState g_NormalSampler : register(s1);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv: UV;
    float3 worldPosition : POSITION;
    float3x3 TBN : TBN;
};

[RootSignature(MyRS1)]
PSInput VS_Main(VSInput input)
{
    PSInput result;
    float2 vUv = input.uv;
    vUv = (vUv * 2.0) - 1.0;

    // matrix mvp = mul(gViewProjection, oLocalToWorld);
    float3 N = normalize(mul((float3x3)oNormalsMatrix, input.normal));

    result.position = float4(vUv, 1.0, 1.0);
    result.worldPosition = mul(oLocalToWorld, float4(input.position, 1.0)).xyz;
    result.normal = N;
    result.uv = input.uv;
    result.uv.y = 1.0 - result.uv.y;

    float3 T = normalize(mul((float3x3)oNormalsMatrix, input.tangent));
    float3 B = normalize(cross(T, N));

    float3x3 TBN = float3x3(T, B, N);
    result.TBN = TBN;

    return result;
}

[RootSignature(MyRS1)]
float4 PS_Main(PSInput input) : SV_TARGET
{
    float4 albedo = albedoTexture.Sample(g_sampler, input.uv);
    float3 metalness = metalnessTexture.Sample(g_sampler, input.uv);
    float3 roughness = roughnessTexture.Sample(g_sampler, input.uv);

    float3 normal = normalTexture.Sample(g_NormalSampler, input.uv).rgb;
    normal = 2 * normal - 1;
    normal = normalize(mul(normal, input.TBN));

    float3 geometricNormal = normalize(input.normal);
    float3 fragmentToLight = normalize(gDirectionalLightPosition - input.worldPosition);
    float3 fragmentToEye = normalize(gCameraPosition - input.worldPosition);
    
    // This is here to prevent the glitches on the back of the model. Since
    // the backfaces are no longer culled.
    float shadowFactor = step(0, dot(geometricNormal, fragmentToLight));
    float3 lightReflect = normalize(reflect(-fragmentToLight, normal));
    float cosOut = max(dot(normal, fragmentToLight), 0);

    // Ambient Light
    float3 ambient = gAmbientLight * gAmbientIntensity;

    // We need to support this from the material side
    float3 F0 = lerp(0.4, albedo, metalness);

}