#define MyRS1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
                        "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
                        "DENY_HULL_SHADER_ROOT_ACCESS | " \
                        "DENY_GEOMETRY_SHADER_ROOT_ACCESS), " \
                "CBV(b0)," \
                "CBV(b1)," \
                "DescriptorTable(SRV(t0, numDescriptors = 2, flags = DESCRIPTORS_VOLATILE), visibility = SHADER_VISIBILITY_PIXEL), " \
                "StaticSampler(s0," \
                      "addressU = TEXTURE_ADDRESS_BORDER," \
                      "addressV = TEXTURE_ADDRESS_BORDER," \
                      "addressW = TEXTURE_ADDRESS_BORDER," \
                      "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK," \
                      "filter = FILTER_MIN_MAG_MIP_POINT, "\
                      "visibility = SHADER_VISIBILITY_PIXEL)"

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

Texture2D g_DiffuseTexture : register(t0);
Texture2D g_NormalTexture : register(t1);
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
    result.normal = input.normal;//  mul(oNormalsMatrix , float4(input.normal, 0.0)).xyz;
    result.uv = input.uv;
    result.uv.y = 1.0 - result.uv.y;

    return result;
}

[RootSignature(MyRS1)]
float4 PS_Main(PSInput input) : SV_TARGET
{
    float4 texColor = g_DiffuseTexture.Sample(g_sampler, input.uv);
    // This won't work until I implement proper normal maps
    // float3 normal = g_NormalTexture.Sample(g_sampler, input.uv);
    // normal = normalize(normal * 2.0 - 1.0);
    // normal = mul(oNormalsMatrix, float4(normal, 0.0)).xyz;

    float3 fragmentToLight = normalize(gDirectionalLightPosition - input.worldPosition);
    float3 fragmentToEye = normalize(gCameraPosition - input.worldPosition);
    float3 normal = normalize(input.normal);

    // Ambient Light
    float3 ambient = gAmbientLight * gAmbientIntensity;

    // Direct Diffuse Light
    float brightness = max(dot(normal, fragmentToLight), 0.0);
    float3 directDiffuseLight = gDirectionalLight * brightness;

    // Direct Specular Light
    /* ===============================================
	 * --[Specular Light]-----------------------------
	 * ===============================================
	 * I = Si * Sc * (R.V)^n
	 *
	 * Where:
	 *
	 * Si = Specular Intensity
	 * Sc = Specular Colour
	 * R = Reflection Vector = 2 * (N.L) * N-L
	 *	   reflect(L, N) = L - 2 * N * dot(L, N) [HLSL Built-in Function]
	 * V = View Vector = Camera.WorldPosition - Pixel.WorldPosition
	 * n = Shininess factor
	 *
	 */
    // reflect wants the incident vector
    float3 lightReflect = normalize(reflect(-fragmentToLight, normal));
    float specularFalloff = max(dot(lightReflect, fragmentToEye), 0.0);
    specularFalloff = pow(specularFalloff, oGlossiness);
    float3 directSpecular = 2.0 * gDirectionalLight * specularFalloff;

    // if (texColor.r != 0.0 || texColor.b != 0.0 || texColor.g != 0.0) {
    //     texColor = directDiffuseLight;
    // }

    // Composite
    float3 diffuseLight = ambient + directDiffuseLight + directSpecular;
    float4 finalSurfaceColor = float4((diffuseLight * texColor.xyz) , texColor.a);

    return finalSurfaceColor;
}