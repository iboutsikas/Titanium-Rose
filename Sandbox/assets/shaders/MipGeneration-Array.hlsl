#include "MipGeneration.common.hlsli"


// Source mip map.
Texture2DArray<float4> SrcMip : register( t0 );
// Write up to 4 mip map levels.
RWTexture2DArray<float4> OutMip1 : register( u0 );
RWTexture2DArray<float4> OutMip2 : register( u1 );
RWTexture2DArray<float4> OutMip3 : register( u2 );
RWTexture2DArray<float4> OutMip4 : register( u3 );
SamplerState defaultSampler : register(s0);


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
void CS_Main(uint3 ThreadID: SV_DispatchThreadID, uint GroupIndex: SV_GroupIndex)
{
    // int4 uv = int4(2 * ThreadID.x, 2 * ThreadID.y, ThreadID.z, 0);
    // float4 Src1 = SrcMip.Load(uv, 0);

    float3 uv = 0;
    uv.xy = TexelSize.zw * (ThreadID.xy + 0.5);
    uv.z = ThreadID.z;

    float4 Src1 = SrcMip.SampleLevel(defaultSampler, uv, SrcMipLevel);

    OutMip1[int3(ThreadID.x, ThreadID.y , ThreadID.z)] = Src1;

    // OutMip1[int3(ThreadID.x,        ThreadID.y    , ThreadID.z)] = float4(1, 1, 1, 1);
    // OutMip2[int3(ThreadID.x / 2,    ThreadID.y / 2, ThreadID.z)] = float4(1, 0, 1, 1);
    // OutMip3[int3(ThreadID.x / 4,    ThreadID.y / 4, ThreadID.z)] = float4(1, 1, 0, 1);
    // OutMip4[int3(ThreadID.x / 8,    ThreadID.y / 8, ThreadID.z)] = float4(0, 1, 1, 1);
    // return;

    if (NumMipLevels == 1) return;

    StoreColor(GroupIndex, Src1);
    GroupMemoryBarrierWithGroupSync();

    if ((GroupIndex & 0x9) == 0) {
        float4 Src2 = LoadColor( GroupIndex + 0x01 );
        float4 Src3 = LoadColor( GroupIndex + 0x08 );
        float4 Src4 = LoadColor( GroupIndex + 0x09 );
        Src1 = 0.25 * ( Src1 + Src2 + Src3 + Src4 );
    
        OutMip2[int3(ThreadID.x / 2, ThreadID.y / 2, ThreadID.z)] = Src1;
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
    
        OutMip3[int3(ThreadID.x / 4, ThreadID.y / 4, ThreadID.z)] = Src1;
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
    
        OutMip4[int3(ThreadID.x / 8, ThreadID.y / 8, ThreadID.z)] = Src1;
        StoreColor( GroupIndex, Src1 );
    }
}