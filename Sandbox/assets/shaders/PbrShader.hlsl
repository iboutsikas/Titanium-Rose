#include "RootSignatures.hlsli"
#include "Common.hlsli"

Texture2D<float4> AlbedoTexture     : register(t0);
Texture2D<float3> NormalTexture     : register(t1);
Texture2D<float3> SpecularTexture   : register(t2);
StructuredBuffer<Light> SceneLights : register(t3);

SamplerState someSampler: register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 w_Pos : POSITION;
    float2 uv: UV;
    float3x3 TBN : TBN;
};

cbuffer cbPerObject : register(b0) {
    matrix LocalToWorld     : packoffset(c0);
    matrix WorldToLocal     : packoffset(c4);
    float4 MaterialColor    : packoffset(c8);
    float4 EmissiveColor    : packoffset(c9);
    float MaterialSpecular  : packoffset(c10.x);
    bool HasAlbedo          : packoffset(c10.y);
    bool HasNormal          : packoffset(c10.z);
    bool HasSpecular        : packoffset(c10.w);
};

cbuffer cbPass : register(b1) {
    matrix ViewProjection       : packoffset(c0);
    float3 AmbientLight         : packoffset(c4.x);
    float AmbientIntensity      : packoffset(c4.w);     
    float3 EyePosition          : packoffset(c5.x);
    uint  NumLights             : packoffset(c5.w);       
};


[RootSignature(PBR_RS)]
PSInput VS_Main(VSInput input)
{
    PSInput result;

    float3 N = normalize(mul((float3x3)WorldToLocal, input.normal));
    float3 T = normalize(mul((float3x3)WorldToLocal, input.tangent));
    float3 B = normalize(cross(T, N));
    float3x3 TBN = float3x3(T, B, N);
    result.TBN = TBN;

    matrix mvp = mul(ViewProjection, LocalToWorld);
    result.position = mul(mvp, float4(input.position, 1.0f));
    result.normal = N;
    result.uv = input.uv;
    result.uv.y = 1.0 - result.uv.y;
    
    result.w_Pos = mul(float4(input.position, 1.0), LocalToWorld).xyz;
    
    return result;
}

[RootSignature(PBR_RS)]
float4 PS_Main(PSInput input) : SV_TARGET
{
    float4 albedo;
    if (HasAlbedo)
    {
        albedo = AlbedoTexture.Sample(someSampler, input.uv);
    }
    else
    {
        albedo = MaterialColor;
    }

    float3 normal;
    float3 geometricNormal = normalize(input.normal);
    if (HasNormal)
    {
        normal = NormalTexture.Sample(someSampler, input.uv);
        normal = 2.0 * normal - 1.0;
        normal = normalize(mul(normal, input.TBN));
    }
    else
    {
        normal = geometricNormal;
    }

    float specular;
    if (HasSpecular)
    {
        // The texture is normalized from 0 - 1. So we need to multiply it by SOME scale.
        // In this case it is the shininess we get from the material
        specular = SpecularTexture.Sample(someSampler, input.uv).r * MaterialSpecular;
    }
    else
    {
        specular = MaterialSpecular;
    }

    float3 ambientContribution = AmbientLight.rgb * AmbientIntensity;

    float3 FragmentToView = normalize(EyePosition - input.w_Pos);

    
    float3 diffuseContribution = float3(0, 0, 0);
    float3 specularContribution = float3(0, 0, 0);

    for (uint i = 0; i < NumLights; i++)
    {
        Light the_light = SceneLights[i];
        

        float3 L = the_light.Position - input.w_Pos;
        float d = length(L);
        if (d > the_light.Range)
        {
            continue;
        }


        float shadowFactor = step(0, dot(geometricNormal, L));
        float attenuation = CalculateAttenuation(the_light, d);
        float3 Lnorm = normalize(L);

        // Diffuse Contribution

        float3 diffContrib = CalculateDiffuse(the_light, Lnorm, normal) * 
                            the_light.Intensity * 
                            attenuation *
                            shadowFactor;
        diffuseContribution += diffContrib;

        float3 specContrib = CalculateSpecular(the_light, FragmentToView, Lnorm, normal) * 
                            the_light.Intensity * 
                            shadowFactor *
                            attenuation;
        specularContribution += specContrib;
    }

    float3 finalSurfaceColor = 0;
    finalSurfaceColor += ambientContribution;
    finalSurfaceColor += diffuseContribution;
    finalSurfaceColor += specularContribution;
    finalSurfaceColor *= albedo.rgb;

    return float4(finalSurfaceColor, albedo.a) + EmissiveColor;
}