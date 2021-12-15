#include "MipGeneration.common.hlsli"


// Source mip map.
Texture2D<float4> SrcMip : register( t0 );
// Write up to 4 mip map levels.
RWTexture2D<float4> OutMip1 : register( u0 );
RWTexture2D<float4> OutMip2 : register( u1 );
RWTexture2D<float4> OutMip3 : register( u2 );
RWTexture2D<float4> OutMip4 : register( u3 );
// Linear clamp sampler.
SamplerState TrillinearClamp : register( s0 );


// The reason for separating channels is to reduce bank conflicts in the
// local data memory controller.  A large stride will cause more threads
// to collide on the same memory bank.
groupshared float gs_R[64];
groupshared float gs_G[64];
groupshared float gs_B[64];
groupshared float gs_A[64];

void StoreColor( uint Index, float4 Color )
{
    gs_R[Index] = Color.r;
    gs_G[Index] = Color.g;
    gs_B[Index] = Color.b;
    gs_A[Index] = Color.a;
}

float4 LoadColor( uint Index )
{
    return float4( gs_R[Index], gs_G[Index], gs_B[Index], gs_A[Index] );
}

[RootSignature(RS)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void CS_Main(uint2 ThreadID: SV_DispatchThreadID, uint GroupIndex: SV_GroupIndex)
{
    float2 uv = TexelSize.zw * (ThreadID.xy + 0.5);
    float4 Src1 = SrcMip.SampleLevel(TrillinearClamp, uv, SrcMipLevel);

    OutMip1[ThreadID.xy] = Src1;
    
    if (NumMipLevels == 1) return;

    StoreColor(GroupIndex, Src1);
    GroupMemoryBarrierWithGroupSync();

    if ((GroupIndex & 0x9) == 0) {
        float4 Src2 = LoadColor( GroupIndex + 0x01 );
        float4 Src3 = LoadColor( GroupIndex + 0x08 );
        float4 Src4 = LoadColor( GroupIndex + 0x09 );
        Src1 = 0.25 * ( Src1 + Src2 + Src3 + Src4 );
    
        OutMip2[ThreadID.xy / 2] = Src1;
        StoreColor( GroupIndex, Src1 );
    }

    if ( NumMipLevels == 2 ) return;
    GroupMemoryBarrierWithGroupSync();

    if ( ( GroupIndex & 0x1B ) == 0 )
    {
        float4 Src2 = LoadColor( GroupIndex + 0x02 );
        float4 Src3 = LoadColor( GroupIndex + 0x10 );
        float4 Src4 = LoadColor( GroupIndex + 0x12 );
        Src1 = 0.25 * ( Src1 + Src2 + Src3 + Src4 );
    
        OutMip3[ThreadID.xy / 4] = Src1;
        StoreColor( GroupIndex, Src1 );
    }

    if ( NumMipLevels == 3 )
        return;
    
    GroupMemoryBarrierWithGroupSync();

    // Don't really need the mask here. It is more for future proofing
    if ( ( GroupIndex & 0xFF ) == 0 )
    {
        float4 Src2 = LoadColor( GroupIndex + 0x04 );
        float4 Src3 = LoadColor( GroupIndex + 0x20 );
        float4 Src4 = LoadColor( GroupIndex + 0x24 );
        Src1 = 0.25 * ( Src1 + Src2 + Src3 + Src4 );
    
        OutMip4[ThreadID.xy / 8] = Src1;
        StoreColor( GroupIndex, Src1 );
    }
}