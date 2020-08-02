#define RS\
"RootFlags(0), " \
"DescriptorTable(UAV(u0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
"DescriptorTable(SRV(t0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
"StaticSampler(s0," \
	"addressU = TEXTURE_ADDRESS_BORDER," \
	"addressV = TEXTURE_ADDRESS_BORDER," \
	"addressW = TEXTURE_ADDRESS_BORDER," \
	"filter = FILTER_MIN_MAG_MIP_LINEAR)"

#include "Environment.common.hlsli"
static const uint NumSamples = 64 * 1024;
static const float InvNumSamples = 1.0 / float(NumSamples);

#define THREAD_BLOCK 32

TextureCube inputTexture : register(t0);
RWTexture2DArray<float4> outputTexture : register(u0);
SamplerState defaultSampler : register(s0);

[RootSignature(RS)]
[numthreads(THREAD_BLOCK, THREAD_BLOCK, 1)]
void CS_Main(uint3 ThreadID : SV_DispatchThreadID)
{
    uint outputWidth, outputHeight, outputDepth;
	outputTexture.GetDimensions(outputWidth, outputHeight, outputDepth);

	float3 N = getSamplingVector(ThreadID, outputWidth, outputHeight);
	
	float3 S, T;
	computeBasisVectors(N, S, T);

	// Monte Carlo integration of hemispherical irradiance.
	// As a small optimization this also includes Lambertian BRDF assuming perfectly white surface (albedo of 1.0)
	// so we don't need to normalize in PBR fragment shader (so technically it encodes exitant radiance rather than irradiance).
	float3 irradiance = 0.0;
	for(uint i=0; i<NumSamples; ++i) {
		float2 u  = sampleHammersley(i, InvNumSamples);
		float3 Li = tangentToWorld(sampleHemisphere(u.x, u.y), N, S, T);
		float cosTheta = max(0.0, dot(Li, N));
		float3 colour = inputTexture.SampleLevel(defaultSampler, Li, 0).rgb;
		// PIs here cancel out because of division by pdf.
		irradiance += 2.0 * colour * cosTheta;
	}
	irradiance /= float(NumSamples);

	outputTexture[ThreadID] = float4(irradiance, 1.0);
}