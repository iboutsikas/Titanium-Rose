struct VSInput
{
    float3 position : POSITION;
    float3 normal: NORMAL;
    float3 tangent: TANGENT;
    float2 uv: UV;
};

float3 TangentNormalToWorldNormal(float3 tangentNormal, float3 normal, float3 tangent)
{
    
}