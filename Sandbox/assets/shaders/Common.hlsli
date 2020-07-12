struct VSInput
{
    float3 position : POSITION;
    float3 normal: NORMAL;
    float3 tangent: TANGENT;
    float3 binormal : BINORMAL;
    float2 uv: UV;
};

struct Light
{
    float4 Position;
    // ( 16 bytes )
    float3 Color;
    float Intensity;
    // ( 16 bytes )
    float Range;
    float3 _padding;
    // ( 16 bytes )
};


float CalculateAttenuation(Light light, float distance)
{
    return 1.0f - smoothstep( light.Range * 0.75f, light.Range, distance );
}

float3 CalculateDiffuse(Light light, float3 L, float3 N)
{
    float NdotL = max(dot(N, L), 0);

    return light.Color * NdotL;
}

float3 CalculateSpecular(Light light, float3 V, float3 L, float3 N, float3 specularPower)
{
    float3 R = normalize(reflect(-L, N));
    float RdotV = max(dot(R, V), 0);
    float factor = max(pow(RdotV, specularPower), 0);
    return light.Color * factor;
}