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
                "DescriptorTable(SRV(t0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE), visibility = SHADER_VISIBILITY_PIXEL) " 

cbuffer cbPass : register(b0) {
    float4x4 gViewProjection;
    float4   gAmbientLight;
    float4   gDirectionalLight;
    float3   gDirectionalLightPosition;
    float    __padding1;
    float3   gCameraPosition;
    float1    gAmbientIntensity;
};

cbuffer cbPerObject : register(b1) {
    float4x4 oLocalToWorld;
    float4x4 oNormalsMatrix;
    float    oGlossiness;
}

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv: UV;
    float3 worldPosition : POSITION;
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
    float2 vUv = input.uv;
    vUv = (vUv * 2.0) - 1.0;
    
    // matrix mvp = mul(gViewProjection, oLocalToWorld);

    result.position = float4(vUv, 1.0, 1.0);
    result.worldPosition = mul(oLocalToWorld, float4(input.position, 1.0)).xyz;
    result.normal = mul(oNormalsMatrix , float4(input.normal, 0.0)).xyz;
    result.uv = input.uv;
    result.uv.y = 1.0 - result.uv.y;

    return result;
}

[RootSignature(MyRS1)]
float4 PS_Main(PSInput input) : SV_TARGET
{
    float4 texColor = g_texture.Sample(g_sampler, input.uv);

    float3 fragmentToLight = normalize(gDirectionalLightPosition - input.worldPosition);
    float3 fragmentToEye = normalize(gCameraPosition - input.worldPosition);
    float3 normal = normalize(input.normal);

    // Ambient Light
    float3 ambient = gAmbientLight * gAmbientIntensity;
    
    // Direct Diffuse Light
    float brightness = max(dot(normal, fragmentToLight), 0.0);
    float3 directDiffuseLight = gDirectionalLight * brightness;
    
    // Direct Specular Light
    // reflect wants the incident vector
    float3 lightReflect = normalize(reflect(-fragmentToLight, normal));
    float specularFalloff = saturate(dot(lightReflect, fragmentToEye));
    specularFalloff = pow(specularFalloff, oGlossiness) * 2.0;
    float3 directSpecular = gDirectionalLight * specularFalloff;

    // if (texColor.r != 0.0 || texColor.b != 0.0 || texColor.g != 0.0) {
    //     texColor = directDiffuseLight;
    // }

    // Composite
    float3 diffuseLight = ambient + directDiffuseLight + directSpecular;
    float4 finalSurfaceColor = float4(diffuseLight * texColor.xyz , texColor.a);
        
    return finalSurfaceColor;
}
