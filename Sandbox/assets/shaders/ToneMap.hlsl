#define RS \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
        "DENY_VERTEX_SHADER_ROOT_ACCESS), " \
"RootConstants(b0, num32BitConstants = 1, visibility = SHADER_VISIBILITY_PIXEL), " \
"DescriptorTable(SRV(t0, numDescriptors = 5, flags = DESCRIPTORS_VOLATILE), visibility = SHADER_VISIBILITY_PIXEL)," \
"StaticSampler(s0," \
        "filter = FILTER_MIN_MAG_MIP_POINT, "\
        "visibility = SHADER_VISIBILITY_PIXEL)"

cbuffer cbPass : register(b0)
{
    float Exposure;
}

Texture2D sceneColor: register(t0);
SamplerState defaultSampler : register(s0);

struct PSInput {
    float4 Position : SV_POSITION;
	float2 UV       : TEXCOORD;
};

[RootSignature(RS)]
PSInput VS_Main(uint vertexID: SV_VertexID)
{
    PSInput result;

    if(vertexID == 0) {
		result.UV = float2(1.0, -1.0);
		result.Position = float4(1.0, 3.0, 0.0, 1.0);
	}
	else if(vertexID == 1) {
		result.UV = float2(-1.0, 1.0);
		result.Position = float4(-3.0, -1.0, 0.0, 1.0);
	}
	else /* if(vertexID == 2) */ {
		result.UV = float2(1.0, 1.0);
		result.Position = float4(1.0, -1.0, 0.0, 1.0);
	}
	return result;
}

[RootSignature(RS)]
float4 PS_Main(PSInput pin) : SV_Target
{
	float3 color = sceneColor.Sample(defaultSampler, pin.UV).rgb * Exposure;
	
    static const float gamma = 2.2;
    static const float pureWhite = 1.0;

	// Reinhard tonemapping operator.
	// see: "Photographic Tone Reproduction for Digital Images", eq. 4
	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance/(pureWhite*pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	float3 mappedColor = (mappedLuminance / luminance) * color;

	// Gamma correction.
	return float4(pow(mappedColor, 1.0/gamma), 1.0);
	// return float4(color, 1);
}
