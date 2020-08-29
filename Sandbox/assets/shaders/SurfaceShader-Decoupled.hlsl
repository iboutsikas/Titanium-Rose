#include "RootSignatures.hlsli"
#include "Surface.common.hlsli"
#include "Environment.common.hlsli"

// Constant normal incidence Fresnel factor for all dielectrics.
static const float3 Fdielectric = 0.04;

Texture2D<float3> AlbedoTexture : register(t0);
Texture2D<float3> NormalTexture : register(t1);
Texture2D<float> MetalnessTexture : register(t2);
Texture2D<float> RoughnessTexture : register(t3);
TextureCube EnvRadianceTexture : register(t4);
TextureCube EnvIrradianceTexture : register(t5);
Texture2D<float2> BRDFLUT : register(t6);
StructuredBuffer<Light> SceneLights : register(t7);

SamplerState someSampler : register(s0);
SamplerState brdfSampler : register(s1);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 WorldPosition : POSITION;
    float2 uv : UV;
    float3x3 TBN : TBN;
};

cbuffer cbPerObject : register(b0)
{
    matrix LocalToWorld;
    matrix WorldToLocal;
    float3 MaterialColor;
    bool HasAlbedo;
    float3 MaterialEmissive;
    bool HasNormal;
    bool HasMetallic;
    float MaterialMetallic;
    bool HasRoughness;
    float MaterialRoughness;
    uint FinestMip;
    uint3 _padding;
};

cbuffer cbPass : register(b1)
{
    matrix ViewProjection : packoffset(c0);
    float3 EyePosition : packoffset(c4.x);
    uint NumLights : packoffset(c4.w);
};

[RootSignature(PBR_RS2)] 
PSInput VS_Main(VSInput input) 
{
    PSInput result;

    float2 vUv = input.uv;
    vUv = (vUv * 2.0) - 1.0;

    float3x3 TBN = float3x3(input.tangent, input.binormal, input.normal);

    result.TBN = mul((float3x3)LocalToWorld, TBN);

    matrix mvp = mul(ViewProjection, LocalToWorld);
    result.position = float4(vUv, 0.0, 1.0);
    result.normal = mul((float3x3)LocalToWorld, input.normal);
    result.uv = float2(input.uv.x, 1.0 - input.uv.y);

    result.WorldPosition = mul(LocalToWorld, float4(input.position, 1.0)).xyz;

    return result;
}

[RootSignature(PBR_RS2)] 
float4 PS_Main(PSInput input) : SV_TARGET
{
    float3 Albedo = HasAlbedo ? AlbedoTexture.Sample(someSampler, input.uv) : MaterialColor;
    float Metalness = HasMetallic ? MetalnessTexture.Sample(someSampler, input.uv).r : MaterialMetallic;
    float Roughness = HasRoughness ? RoughnessTexture.Sample(someSampler, input.uv).r : MaterialRoughness;

    float3 Normal = normalize(input.normal);

    if (HasNormal)
    {
        float3 N = NormalTexture.Sample(someSampler, input.uv).rgb;
        N = 2.0 * N - 1.0;
        Normal = normalize(mul(N, input.TBN));
    }

    // return float4(input.uv, 0, 1);

    float3 FragmentToCamera = normalize(EyePosition - input.WorldPosition);
    float cosLo = max(dot(Normal, FragmentToCamera), 0.0);

    // Specular reflection vector.
    float3 Lr = 2.0 * cosLo * Normal - FragmentToCamera;

    // Fresnel reflectance at normal incidence (for metals use albedo color).
    float3 F0 = lerp(Fdielectric, Albedo, Metalness);

    float3 directLighting = 0.0;
#if 1
    for (uint i = 0; i < NumLights; i++)
    {
        Light l = SceneLights[i];

        float3 V = l.Position.xyz - input.WorldPosition;

        float d = length(V);

        if (d > l.Range)
        {
            continue;
        }

        float attenuation = CalculateAttenuation(l.Range, d);

        float shadowFactor = step(0, dot(input.normal, FragmentToCamera));

        float3 Li = normalize(V);

        // Half-vector between Li and Lo.
        float3 Lh = normalize(Li + FragmentToCamera);

        // Calculate angles between surface normal and various light vectors.
        float cosLi = max(0.0, dot(Normal, Li));
        float cosLh = max(0.0, dot(Normal, Lh));

        float3 F = fresnelSchlick(F0, max(dot(Lh, FragmentToCamera), 0.0));

        float D = ndfGGX(cosLh, Roughness);

        float G = gaSchlickGGX(cosLi, cosLo, Roughness);

        float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), Metalness);

        // Lambert diffuse BRDF.
        // We don't scale by 1/PI for lighting & material units to be more convenient.
        // See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
        float3 diffuseBRDF = kd * Albedo;

        // Cook-Torrance specular microfacet BRDF.
        float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

        // Total contribution for this light.
        directLighting += (diffuseBRDF + specularBRDF) * l.Color * l.Intensity * cosLi * attenuation * shadowFactor;
    }
#endif
    // Ambient lighting (IBL).
    float3 ambientLighting;
    {
        // Sample diffuse irradiance at normal direction.
        float3 irradiance = EnvIrradianceTexture.Sample(someSampler, Normal).rgb;

        // Calculate Fresnel term for ambient lighting.
        // Since we use pre-filtered cubemap(s) and irradiance is coming from many directions
        // use cosLo instead of angle with light's half-vector (cosLh above).
        // See: https://seblagarde.wordpress.com/2011/08/17/hello-world/
        float3 F = fresnelSchlick(F0, cosLo);

        // Get diffuse contribution factor (as with direct lighting).
        float3 kd = lerp(1.0 - F, 0.0, Metalness);

        // Irradiance map contains exitant radiance assuming Lambertian BRDF, no need to scale by 1/PI here either.
        float3 diffuseIBL = kd * Albedo * irradiance;

        uint width, height, levels;
        EnvRadianceTexture.GetDimensions(0, width, height, levels);

        // Split-sum approximation factors for Cook-Torrance specular BRDF.
        float3 specularIrradiance = EnvRadianceTexture.SampleLevel(someSampler, Lr, Roughness * levels).rgb;

        // Split-sum approximation factors for Cook-Torrance specular BRDF.
        float2 specularBRDF = BRDFLUT.Sample(brdfSampler, float2(cosLo, Roughness)).rg;
        // Total specular IBL contribution.
        float3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

        // Total ambient lighting contribution.
        ambientLighting = diffuseIBL + specularIBL;
    }

    // return float4(1, 0, 0, 1);
    // return float4(ambientLighting + MaterialEmissive, 1.0);
    return float4(directLighting + ambientLighting + MaterialEmissive, 1);
}