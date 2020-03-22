#define MyRS1 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
                        "DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
                        "DENY_HULL_SHADER_ROOT_ACCESS ), " \
                "CBV(b0)," \
                "CBV(b1)" 

cbuffer cbPass : register(b0) {
    matrix gViewProjection;
    float4 gNormalColor;
    float  gNormalLength;
};

cbuffer cbPerObject : register(b1) {
    float4x4 oLocalToWorld;
    matrix oNormalsMatrix;
}

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

[RootSignature(MyRS1)]
[maxvertexcount(6)]
void GS_Main(triangle VSOutput input[3], inout LineStream<GSOutput> lineStream)
{
    uint i;
    
    [unroll]
    GSOutput gsout;
    for (i = 0; i < 3; i++) {
        // gsout.position = input[i].position;
        // lineStream.Append(gsout);
        GSOutput vtx1;
        // This is the point ON the vertex
        
        vtx1.position = input[i].position;
        lineStream.Append(vtx1);

        GSOutput vtx2;
        // This is the other point of the line
        float3 p2 = (input[i].worldPosition + input[i].normal) * gNormalLength;
        vtx2.position = mul(gViewProjection, float4(p2, 1.0));
        lineStream.Append(vtx2);
        lineStream.RestartStrip();
    }
}

[RootSignature(MyRS1)]
float4 PS_Main(GSOutput input) : SV_TARGET
{
    return gNormalColor;
}