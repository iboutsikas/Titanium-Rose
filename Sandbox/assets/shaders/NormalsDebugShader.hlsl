#define MyRS1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
                        "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
                        "DENY_HULL_SHADER_ROOT_ACCESS ), " \
                "CBV(b0)," \
                "CBV(b1)" 

cbuffer cbPass : register(b0) {
    matrix gViewProjection;
    float4 gNormalColor;
    float4 gReflectColor;
    float3 gLightPosition;
    float  gNormalLength;
};

cbuffer cbPerObject : register(b1) {
    matrix oLocalToWorld;
    matrix oNormalsMatrix;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal: NORMAL;
    float2 uv: UV;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition: POSITION;
    float3 normal: NORMAL;
};

struct GSOutput {
    float4 position: SV_POSITION;
    float   isNormal: ISNORMAL;
};


[RootSignature(MyRS1)]
VSOutput VS_Main(VSInput input)
{
    VSOutput result;
    matrix mvp = mul(gViewProjection, oLocalToWorld);
    
    result.position = mul(mvp, float4(input.position, 1.0));
    result.worldPosition = mul(oLocalToWorld, float4(input.position, 1.0)).xyz;
    result.normal = mul(oNormalsMatrix, float4(input.normal, 0.0)).xyz;
    return result;
}

#define ENABLE_NORMALS 1
#define ENABLE_REFLECT 0
[RootSignature(MyRS1)]
[maxvertexcount(6)]
void GS_Main(triangle VSOutput input[3], inout LineStream<GSOutput> lineStream)
{
    uint i;
    
    GSOutput gsout;
    for (i = 0; i < 3; i++) {
#if ENABLE_NORMALS
        GSOutput vtx1;
        // This is the point ON the vertex
        vtx1.position = input[i].position;
        vtx1.position.z = 0.1;
        vtx1.isNormal = 1.0f;
        lineStream.Append(vtx1);

        GSOutput vtx2;
        // This is the other point of the line
        float3 p2 = input[i].worldPosition + mul(input[i].normal, gNormalLength);
        vtx2.position = mul(gViewProjection, float4(p2, 1.0));
        vtx2.isNormal = 1.0f;
        vtx2.position.z = 0.1;
        lineStream.Append(vtx2);
        lineStream.RestartStrip();
#endif

#if ENABLE_REFLECT
        // These are the reflect vectors
        GSOutput vtx3;
        vtx3.position = input[i].position;
        vtx3.isNormal = 0.0f;
        vtx3.position.z = 0.1;
        lineStream.Append(vtx3);

        float3 toLight = normalize(gLightPosition - input[i].worldPosition);
        float3 lightReflect = normalize(reflect(-toLight, input[i].normal));
        GSOutput vtx4;
        float3 p3 = input[i].worldPosition + mul(lightReflect, gNormalLength);
        vtx4.position = mul(gViewProjection, float4(p3, 1.0));
        vtx4.isNormal = 0.0f;
        vtx4.position.z = 0.1;
        lineStream.Append(vtx4);
        lineStream.RestartStrip();
#endif
    }
}

[RootSignature(MyRS1)]
float4 PS_Main(GSOutput input) : SV_TARGET
{
    return (gNormalColor * input.isNormal) + (gReflectColor * (1.0 - input.isNormal));
}