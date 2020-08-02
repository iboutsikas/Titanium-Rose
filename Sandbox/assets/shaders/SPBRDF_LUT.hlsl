#define RS \
"RootFlags(0), " \
"DescriptorTable(UAV(u0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
"DescriptorTable(SRV(t0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
"StaticSampler(s0," \
	"addressU = TEXTURE_ADDRESS_BORDER," \
	"addressV = TEXTURE_ADDRESS_BORDER," \
	"addressW = TEXTURE_ADDRESS_BORDER," \
	"filter = FILTER_MIN_MAG_MIP_LINEAR)"

#include "Environment.common.hlsli"

#define BLOCK_SIZE 32
static const uint NumSamples = 1024;
static const float InvNumSamples = 1.0 / float(NumSamples);

RWTexture2D<float2> LUT : register(u0);

[RootSignature(RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void CS_Main(uint2 ThreadID : SV_DispatchThreadID)
{
    // Get output LUT dimensions.
	float outputWidth, outputHeight;
	LUT.GetDimensions(outputWidth, outputHeight);

	// Get integration parameters.
	float cosLo = ThreadID.x / outputWidth;
	float roughness = ThreadID.y / outputHeight;

	// Make sure viewing angle is non-zero to avoid divisions by zero (and subsequently NaNs).
	cosLo = max(cosLo, Epsilon);

	// Derive tangent-space viewing vector from angle to normal (pointing towards +Z in this reference frame).
	float3 Lo = float3(sqrt(1.0 - cosLo*cosLo), 0.0, cosLo);

	// We will now pre-integrate Cook-Torrance BRDF for a solid white environment and save results into a 2D LUT.
	// DFG1 & DFG2 are terms of split-sum approximation of the reflectance integral.
	// For derivation see: "Moving Frostbite to Physically Based Rendering 3.0", SIGGRAPH 2014, section 4.9.2.
	float DFG1 = 0;
	float DFG2 = 0;

	for(uint i=0; i<NumSamples; ++i) {
		float2 u = sampleHammersley(i, InvNumSamples);

		// Sample directly in tangent/shading space since we don't care about reference frame as long as it's consistent.
		float3 Lh = sampleGGX(u.x, u.y, roughness);

		// Compute incident direction (Li) by reflecting viewing direction (Lo) around half-vector (Lh).
		float3 Li = 2.0 * dot(Lo, Lh) * Lh - Lo;

		float cosLi   = Li.z;
		float cosLh   = Lh.z;
		float cosLoLh = max(dot(Lo, Lh), 0.0);

		if(cosLi > 0.0) {
			float G  = gaSchlickGGX_IBL(cosLi, cosLo, roughness);
			float Gv = G * cosLoLh / (cosLh * cosLo);
			float Fc = pow(1.0 - cosLoLh, 5);

			DFG1 += (1 - Fc) * Gv;
			DFG2 += Fc * Gv;
		}
	}

	LUT[ThreadID] = float2(DFG1, DFG2) * InvNumSamples;
}