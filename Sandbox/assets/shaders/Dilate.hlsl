#define RS\
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 8), " \
    "DescriptorTable(SRV(t0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
    "DescriptorTable(UAV(u0, numDescriptors = 1, flags = DESCRIPTORS_VOLATILE))," \
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT)"

static const float2 offsets[8] = {
    float2(-1, 0), float2(1, 0), float2(0, 1), float2(0, -1),
    float2(-1, 1), float2(1, 1), float2(1, -1), float2(-1, 1)
};
static const uint OFFSET_COUNT = 8;

#define BLOCK_SIZE 8

cbuffer GernerateMipsCB: register(b0)
{
    float4  TexelSize;      // width, height, 1 / idth, 1 / height
}

Texture2D<float4> Temporary : register( t0 );
RWTexture2D<float4> Target  : register( u0 );
// Linear clamp sampler.
SamplerState Sampler : register( s0 );

[RootSignature(RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void CS_Main(uint2 ThreadID: SV_DispatchThreadID, uint GroupIndex: SV_GroupIndex)
{
    uint maxSteps = max(uint(TexelSize.x) >> 7, 1);
    float minDistance = 10000.0;
    float2 uv = TexelSize.zw * (ThreadID.xy);

    float4 currentSample = Temporary.SampleLevel(Sampler, uv, 0);
    
    if (currentSample.a == 0)
    {
        for (uint step = 0; step < maxSteps; step++)
        {
            for (uint o = 0; o < OFFSET_COUNT; o++)
            {
                float2 curUV = uv + offsets[o] * TexelSize.zw * step;
                float4 offsetSample = Temporary.SampleLevel(Sampler, curUV, 0);

                if (offsetSample.a != 0) {
                    float distance = length(uv - curUV);

                    if (distance < minDistance) {
                        float2 projectedUV = curUV + (offsets[o] * TexelSize.zw * step);
                        float4 direction = Temporary.SampleLevel(Sampler, projectedUV, 0);
                        minDistance = distance;
                        float4 delta = offsetSample - direction;
                        
                        // currentSample = (direction.a != 0) ? float4(1, 0, 1, 1) : float4(0, 1, 0, 1);
                        currentSample = any(direction) ? offsetSample : offsetSample;
                    }
                }
            }
        }
    }

    Target[ThreadID.xy] = currentSample;
}