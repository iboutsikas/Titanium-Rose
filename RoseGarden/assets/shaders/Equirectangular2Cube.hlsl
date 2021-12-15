#define RS \
"RootFlags(0), " \
"DescriptorTable(SRV(t0, numDescriptors = 1, flags = DATA_STATIC))," \
"DescriptorTable(UAV(u0, numDescriptors = 1, flags = DATA_VOLATILE))," \
"StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_LINEAR)"

#define BLOCK_SIZE 32
static const float PI = 3.141592;
static const float TwoPI = 2 * PI;

struct CS_Input 
{
    uint3 GroupID           : SV_GroupID;
    uint3 GroupThreadID     : SV_GroupThreadID ;
    uint3 DispatchThreadID  : SV_DispatchThreadID;
    uint GroupIndex         : SV_GroupIndex ;
};



Texture2D inputTexture : register(t0);
RWTexture2DArray<float4> outputTexture : register(u0);
SamplerState defaultSampler : register(s0);

float3 getSamplingVector(uint3 ThreadID)
{
	float outputWidth, outputHeight, outputDepth;
	outputTexture.GetDimensions(outputWidth, outputHeight, outputDepth);

    float2 st = ThreadID.xy/float2(outputWidth, outputHeight);
    float2 uv = 2.0 * float2(st.x, 1.0-st.y) - float2(1.0, 1.0);

	// Select vector based on cubemap face index.
	float3 ret;
	switch(ThreadID.z)
	{
	case 0: ret = float3(1.0,  uv.y, -uv.x); break;
	case 1: ret = float3(-1.0, uv.y,  uv.x); break;
	case 2: ret = float3(uv.x, 1.0, -uv.y); break;
	case 3: ret = float3(uv.x, -1.0, uv.y); break;
	case 4: ret = float3(uv.x, uv.y, 1.0); break;
	case 5: ret = float3(-uv.x, uv.y, -1.0); break;
	}
    return normalize(ret);
}


[RootSignature( RS )]
[numthreads( BLOCK_SIZE, BLOCK_SIZE, 1 )]
void CS_Main( CS_Input IN )
{   
    float3 v = getSamplingVector(IN.DispatchThreadID);
	
	// Convert Cartesian direction vector to spherical coordinates.
	float phi   = atan2(v.z, v.x);
	float theta = acos(v.y);

	// Sample equirectangular texture.
	float4 color = inputTexture.SampleLevel(defaultSampler, float2(phi/TwoPI, theta/PI), 0);

	// Write out color to output cubemap.
	outputTexture[IN.DispatchThreadID] = color;
}