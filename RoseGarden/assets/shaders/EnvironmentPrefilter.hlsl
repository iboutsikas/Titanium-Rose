// The SRV is set last since it doesn't change across invocations

#define RS \
"RootFlags(0), " \
"RootConstants(b0, num32BitConstants = 1), " \
"DescriptorTable(UAV(u0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
"DescriptorTable(SRV(t0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
"StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_LINEAR)"

#include "Environment.common.hlsli"

#define THREAD_BLOCK 32

static const uint NumSamples = 1024;
static const float InvNumSamples = 1.0 / float(NumSamples);

cbuffer cbPass : register(b0)
{
    float Roughness;
}

TextureCube inputTexture : register(t0);
RWTexture2DArray<float4> outputTexture : register(u0);
SamplerState defaultSampler : register(s0);

[RootSignature(RS)]
[numthreads(THREAD_BLOCK, THREAD_BLOCK, 1)]
void CS_Main(uint3 ThreadID : SV_DispatchThreadID)
{
    // Make sure we won't write past output when computing higher mipmap levels.
	uint outputWidth, outputHeight, outputDepth;
	outputTexture.GetDimensions(outputWidth, outputHeight, outputDepth);
	if(ThreadID.x >= outputWidth || ThreadID.y >= outputHeight) {
		return;
	}
	
	// Get input cubemap dimensions at zero mipmap level.
	float inputWidth, inputHeight, inputLevels;
	inputTexture.GetDimensions(0, inputWidth, inputHeight, inputLevels);

	// Solid angle associated with a single cubemap texel at zero mipmap level.
	// This will come in handy for importance sampling below.
	float wt = 4.0 * PI / (6 * inputWidth * inputHeight);
	
	// Approximation: Assume zero viewing angle (isotropic reflections).
	float3 N = getSamplingVector(ThreadID, outputWidth, outputHeight);
	float3 Lo = N;
	
	float3 S, T;
	computeBasisVectors(N, S, T);

	float3 color = 0;
	float weight = 0;

	// Convolve environment map using GGX NDF importance sampling.
	// Weight by cosine term since Epic claims it generally improves quality.
	for(uint i=0; i<NumSamples; ++i) {
		float2 u = sampleHammersley(i, InvNumSamples);
		float3 Lh = tangentToWorld(sampleGGX(u.x, u.y, Roughness), N, S, T);

		// Compute incident direction (Li) by reflecting viewing direction (Lo) around half-vector (Lh).
		float3 Li = 2.0 * dot(Lo, Lh) * Lh - Lo;

		float cosLi = dot(N, Li);
		if(cosLi > 0.0) {
			// Use Mipmap Filtered Importance Sampling to improve convergence.
			// See: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html, section 20.4

			float cosLh = max(dot(N, Lh), 0.0);

			// GGX normal distribution function (D term) probability density function.
			// Scaling by 1/4 is due to change of density in terms of Lh to Li (and since N=V, rest of the scaling factor cancels out).
			float pdf = ndfGGX(cosLh, Roughness) * 0.25;

			// Solid angle associated with this sample.
			float ws = 1.0 / (NumSamples * pdf);

			// Mip level to sample from.
			float mipLevel = max(0.5 * log2(ws / wt) + 1.0, 0.0);

			color  += inputTexture.SampleLevel(defaultSampler, Li, mipLevel).rgb * cosLi;
			weight += cosLi;
		}
	}
	color /= weight;

	outputTexture[ThreadID] = float4(color, 1.0);
}
